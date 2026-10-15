/* PACK.CPP: VRAM texture-page packing: the free and full block lists, finding space for a
   texture, merging and defragmenting (declarations in game/pack.h). */
#include "game/pack.h"
#include "game/m3d.h"

#include "game/mem.h"
#include "game/mess.h"
#include "game/shellutils.h"

extern int TotSlots;
extern int Pack_Width;
extern int LastPackInitType;
extern int ZapCalls;
TSPack *Pack_GetSlot(void);
TSPack *Pack_New(TSPack **);
TSPack *Pack_NewFree(int, int, int, int);
void Pack_Remove(TSPack **, TSPack *);
int Pack_Merge(TSPack *);
TSPack *Pack_MergeAdjacent(TSPack *, TSPack *);
void Pack_Zap(int, int, int, int);

/* Pack_GetSlot__Fv (0x800495c4) */
/* a new TSPack (its callers use what Mem_NewX returns, which retail's v0 still held) */
TSPack *Pack_GetSlot(void) {
	TSPack *p = (TSPack *)Mem_NewX(20, 0, 1);

	TotSlots = TotSlots + 1;
	return p;
}

/* Pack_KillSlot__FP6TSPack (0x800495f4) */
void Pack_KillSlot(TSPack *p) {
	Mem_Delete(p);

	TotSlots = TotSlots - 1;
}

/* Pack_New__FPP6TSPack (0x8004961c) */
TSPack *Pack_New(TSPack **list) {
	TSPack *p = Pack_GetSlot();

	p->pNext = *list;
	*list = p;
	return p;
}

/* Pack_NewFree__Fiiii (0x80049650) */
TSPack *Pack_NewFree(int x, int y, int w, int h) {
	TSPack *p = Pack_New(&Pack_pFree);

	p->X = x;
	p->Y = y;
	p->W = w;
	p->H = h;
	return p;
}

/* Pack_NewFull__Fiiii (0x800496b0) */
TSPack *Pack_NewFull(int a, int b, int c, int d) {
	TSPack *p = Pack_New(&Pack_pFull);

	p->X = a;
	p->Y = b;
	p->W = c;
	p->H = d;
	return p;
}

/* Pack_Remove__FPP6TSPackP6TSPack (0x80049710) */
void Pack_Remove(TSPack **list, TSPack *p) {
	TSPack *q;

	if (*list == p) {
		*list = p->pNext;
	} else {
		q = *list;

		while (q->pNext != p)
			q = q->pNext;

		q->pNext = p->pNext;
	}

	Pack_KillSlot(p);
}

/* Pack_Init__Fv (0x80049780) */
void Pack_Init(void) {
	TotSlots = 0;
	Pack_pFree = 0;
	Pack_pFull = 0;
	Pack_Width = 512;
	Pack_NewFree(512, 256, 512, 256);
	Pack_NewFree(512, 0, 512, 256);

	LastPackInitType = 0;
}

/* Pack_Init640x480__Fv (0x800497d4) */
void Pack_Init640x480(void) {
	TotSlots = 0;
	Pack_Width = 640;
	Pack_pFree = 0;
	Pack_pFull = 0;
	Pack_NewFree(640, 256, 384, 256);
	Pack_NewFree(640, 0, 384, 256);

	LastPackInitType = 1;
}

/* Pack_Pack__FiiiPiT3iiUi (0x8004982c) */
/* Demo twin (0.97 by opcode; THPS1 also records the requested size in the block);
   locals from its MAIN.SYM. Finds VRAM space for a W x H texture of the given depth: aligned
   requests first try an exact fit, else carve an aligned slot out of a bigger (or a fresh 256x256)
   allocation and free the leftovers; otherwise pick the smallest free block that fits (from the
   left, then from the right of a block, then any block with an aligned start), split it and
   return the new full block. AlignedX/AlignedW are one pair for the third search and the split
   after it, so AlignedW lives across the Pack_NewFree call in s0, as retail's does. */
TSPack *Pack_Pack(int Type, int WW, int H, int *X, int *Y, int BPP, int Aligned, uint checksum) {
	TSPack *pBest;
	TSPack *pPack;
	int OX;
	int OY;
	int OW;
	int OH;
	int Over;
	int W = WW;

	if (Aligned > 0) {
		if (W == 256 && H == 256)
			Aligned = 0;
	}

	if (Aligned > 0) {
		pPack = Pack_Pack(Type, WW, H, X, Y, BPP, -1, checksum);

		if (!pPack) {
			int NewX;
			int NewY;
			int OldX;
			int OldY;
			int OldW;
			int OldH;

			W = WW;

			if (W == 256 || H == 256) {
				pPack = Pack_Pack(Type, 256, 256, &NewX, &NewY, BPP, 0, checksum);

				if (!pPack)
					return 0;

				OldW = 128;
				OldH = 256;

				if (BPP != 16)
					W = (W + 1) / 2;

				if (BPP == 4) {
					W = (W + 1) / 2;
					OldW = 64;
				}

				OldX = pPack->X;
				OldY = pPack->Y;
			} else {
				pPack = Pack_Pack(Type, W * 2, H * 2, &NewX, &NewY, BPP, 0, checksum);

				if (!pPack)
					return 0;

				if (BPP != 16)
					W = (W + 1) / 2;

				if (BPP == 4)
					W = (W + 1) / 2;

				NewX = (NewX + W - 1) & -W;
				NewY = (NewY + H - 1) & -H;
				OldX = pPack->X;
				OldY = pPack->Y;
				OldW = W * 2;
				OldH = H * 2;
			}

			pPack->X = NewX;
			pPack->Y = NewY;
			pPack->W = W;
			pPack->H = H;
			*X = NewX;
			*Y = NewY;

			if (OldX < NewX) {
				Pack_Merge(Pack_NewFree(OldX, OldY, NewX - OldX, OldH));

				OldW -= NewX - OldX;
			}

			if (W < OldW)
				Pack_Merge(Pack_NewFree(NewX + W, OldY, OldW - W, OldH));

			if (OldY < NewY) {
				Pack_Merge(Pack_NewFree(NewX, OldY, W, NewY - OldY));

				OldH -= NewY - OldY;
			}

			if (H < OldH)
				Pack_Merge(Pack_NewFree(NewX, NewY + H, W, OldH - H));
		}

		pPack->ReqW = WW;
		pPack->ReqH = H;
		pPack->bpp = BPP;
		pPack->aligned = Aligned;
		pPack->Type = Type;
		pPack->no_bottom = Pack_NoBottom;
		return pPack;
	}

	if (BPP != 16)
		W = (W + 1) / 2;

	if (BPP == 4)
		W = (W + 1) / 2;

	pBest = 0;

	for (pPack = Pack_pFree; pPack; pPack = pPack->pNext) {
		int RightEdge;

		if (BPP == 16)
			RightEdge = (pPack->X & ~63) + 256;
		else if (BPP == 8)
			RightEdge = (pPack->X & ~63) + 128;
		else
			RightEdge = (pPack->X & ~63) + 64;

		Over = pPack->X + W > RightEdge;

		if (!Over && Aligned == -1) {
			if ((pPack->X & (W - 1)) || (pPack->Y & (H - 1)))
				Over = 1;
		}

		if (Pack_NoBottom && !((pPack->Y + H) & 255))
			continue;

		if (Pack_NoRight && pPack->X + W == RightEdge)
			continue;

		if (Type >= 0 && Over)
			continue;

		if (pPack->W == W && pPack->H == H) {
			pBest = pPack;
			break;
		}

		if (pBest && (pBest->W == W || pBest->H == H) && pPack->W != W && pPack->H != H)
			continue;

		if (pPack->W >= W && pPack->H >= H && (!pBest || pPack->W * pPack->H < pBest->W * pBest->H))
			pBest = pPack;
	}

	if (!pBest) {
		for (pPack = Pack_pFree; pPack; pPack = pPack->pNext) {
			int RightEdge;

			if (BPP == 16)
				RightEdge = ((pPack->X + pPack->W - W) & ~63) + 256;
			else if (BPP == 8)
				RightEdge = ((pPack->X + pPack->W - W) & ~63) + 128;
			else
				RightEdge = ((pPack->X + pPack->W - W) & ~63) + 64;

			Over = pPack->X + pPack->W > RightEdge;

			if (!Over && Aligned == -1) {
				if (((pPack->X - W) & (W - 1)) || (pPack->Y & (H - 1)))
					Over = 1;
			}

			if (Pack_NoBottom && !((pPack->Y + H) & 255))
				continue;

			if (Pack_NoRight && pPack->X + pPack->W == RightEdge)
				continue;

			if (Type >= 0 && Over)
				continue;

			if (pPack->W == W && pPack->H == H) {
				pBest = pPack;
				continue;
			}

			if (pBest && (pBest->W == W || pBest->H == H) && pPack->W != W && pPack->H != H)
				continue;

			if (pPack->W >= W && pPack->H >= H &&
			    (!pBest || pPack->W * pPack->H < pBest->W * pBest->H))
				pBest = pPack;
		}

		if (!pBest) {
			int AlignedX;
			int AlignedW;

			if (Aligned == -1)
				return 0;

			for (pPack = Pack_pFree; pPack; pPack = pPack->pNext) {
				if (Pack_NoBottom && !((pPack->Y + H) & 255))
					continue;

				if (Pack_NoRight && WW == 256)
					continue;

				if (pBest && (pBest->W == W || pBest->H == H) && pPack->W != W && pPack->H != H)
					continue;

				AlignedX = (pPack->X + 64) & ~63;
				AlignedW = pPack->W - (AlignedX - pPack->X);

				if (AlignedW >= W && pPack->H >= H &&
				    (!pBest || pPack->W * pPack->H < pBest->W * pBest->H))
					pBest = pPack;
			}

			if (!pBest)
				return 0;

			AlignedX = (pBest->X + 64) & ~63;
			AlignedW = pBest->W - (AlignedX - pBest->X);

			if (AlignedW > 0) {
				Pack_NewFree(AlignedX, pBest->Y, AlignedW, pBest->H);

				pBest->W -= AlignedW;
			}

			pPack = Pack_Pack(Type, WW, H, X, Y, BPP, 0, checksum);
			pPack->ReqW = WW;
			pPack->ReqH = H;
			pPack->bpp = BPP;
			pPack->aligned = Aligned;
			pPack->Type = Type;
			pPack->no_bottom = Pack_NoBottom;
			return pPack;
		}

		OX = pBest->X;
		OY = pBest->Y;
		OW = pBest->W;
		OH = pBest->H;
		Pack_Remove(&Pack_pFree, pBest);

		pBest = Pack_NewFull(OX + OW - W, OY, W, H);

		if (OW != W) {
			if (OH != H) {
				Pack_Merge(Pack_NewFree(OX, OY, OW - W, OH));
				Pack_Merge(Pack_NewFree(OX + OW - W, OY + H, W, OH - H));
			} else {
				Pack_Merge(Pack_NewFree(OX, OY, OW - W, OH));
			}
		} else {
			if (OH != H)
				Pack_Merge(Pack_NewFree(OX, OY + H, OW, OH - H));
		}

		*X = OX + OW - W;
		*Y = OY;
	} else {
		OX = pBest->X;
		OY = pBest->Y;
		OW = pBest->W;
		OH = pBest->H;
		Pack_Remove(&Pack_pFree, pBest);

		pBest = Pack_NewFull(OX, OY, W, H);

		if (OW != W) {
			if (OH != H) {
				Pack_Merge(Pack_NewFree(OX + W, OY, OW - W, OH));
				Pack_Merge(Pack_NewFree(OX, OY + H, W, OH - H));
			} else {
				Pack_Merge(Pack_NewFree(OX + W, OY, OW - W, OH));
			}
		} else {
			if (OH != H)
				Pack_Merge(Pack_NewFree(OX, OY + H, OW, OH - H));
		}

		*X = OX;
		*Y = OY;
	}

	pBest->ReqW = WW;
	pBest->ReqH = H;
	pBest->bpp = BPP;
	pBest->aligned = Aligned;
	pBest->Type = Type;
	pBest->no_bottom = Pack_NoBottom;
	return pBest;
}

/* Pack_Unpack__Fss (0x8004a2d4) */
void Pack_Unpack(short x, short y) {
	TSPack *p;

	for (p = Pack_pFull; p; p = p->pNext) {
		if (p->X == x && p->Y == y) {
			int w = p->W, h = p->H;

			Pack_Remove(&Pack_pFull, p);
			Pack_Merge(Pack_NewFree(x, y, w, h));
			return;
		}
	}
}

/* Pack_MergeAdjacent__FP6TSPackT0 (0x8004a388) */
TSPack *Pack_MergeAdjacent(TSPack *p_first, TSPack *p_second) {
	int x, y, w, h;

	if (p_first->Y == p_second->Y) {
		if (p_first->X < p_second->X) {
			if (p_first->X + p_first->W == p_second->X && p_first->H == p_second->H) {
				x = p_first->X;
				y = p_first->Y;
				w = p_first->W + p_second->W;
				h = p_first->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		} else if (p_second->X < p_first->X) {
			if (p_second->X + p_second->W == p_first->X && p_first->H == p_second->H) {
				x = p_second->X;
				y = p_second->Y;
				w = p_first->W + p_second->W;
				h = p_first->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		}
	} else if (p_first->X == p_second->X) {
		if (p_first->Y < p_second->Y) {
			if (p_first->Y + p_first->H == p_second->Y && p_first->W == p_second->W) {
				x = p_first->X;
				y = p_first->Y;
				w = p_first->W;
				h = p_first->H + p_second->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		} else if (p_second->Y < p_first->Y) {
			if (p_second->Y + p_second->H == p_first->Y && p_first->W == p_second->W) {
				x = p_second->X;
				y = p_second->Y;
				w = p_first->W;
				h = p_first->H + p_second->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		}
	}
}

/* Pack_Merge__FP6TSPack (0x8004a590) */
int Pack_Merge(TSPack *p_merge) {
	TSPack *p_free;
	int worth_continuing;
	int merged;

	merged = 0;

	do {
		worth_continuing = 0;
		p_free = Pack_pFree;

		while (p_free) {
			if (p_merge->Y == p_free->Y && p_merge->H == p_free->H) {
				if (p_free->X + p_free->W == p_merge->X || p_merge->X + p_merge->W == p_free->X) {
					worth_continuing = 1;
					p_merge = Pack_MergeAdjacent(p_free, p_merge);
					merged = 1;
					break;
				}
			} else if (p_merge->X == p_free->X && p_merge->W == p_free->W &&
			           (p_free->Y & 256) == (p_merge->Y & 256)) {
				if (p_free->Y + p_free->H == p_merge->Y || p_merge->Y + p_merge->H == p_free->Y) {
					worth_continuing = 1;
					p_merge = Pack_MergeAdjacent(p_free, p_merge);
					merged = 1;
					break;
				}
			}

			p_free = p_free->pNext;
		}
	} while (worth_continuing);

	return merged;
}

/* Pack_Zap__Fiiii (0x8004a69c) */
void Pack_Zap(int x, int y, int w, int h) {
	TSPack *p_loop;
REDO:
	p_loop = Pack_pFull;

	while (p_loop) {
		if (p_loop->X + (p_loop->W - 1) >= x && p_loop->X < x + w &&
		    p_loop->Y + (p_loop->H - 1) >= y && p_loop->Y < y + h)
			break;

		p_loop = p_loop->pNext;
	}

	if (p_loop == 0) {
		Pack_Merge(Pack_NewFree(x, y, w, h));
		return;
	}

	if (x >= p_loop->X && y >= p_loop->Y && p_loop->X + p_loop->W >= x + w &&
	    p_loop->Y + p_loop->H >= y + h)
		return;

	if (w < 32 && h < 32) {
		if (x < p_loop->X) {
			int half = p_loop->X - x;

			Pack_Zap(x, y, half, h);

			x += half;
			w -= half;
			goto REDO;
		}

		if (p_loop->X + p_loop->W < x + w) {
			int half = p_loop->X + p_loop->W - x;

			Pack_Zap(x, y, half, h);

			x += half;
			w -= half;
			goto REDO;
		}

		if (y < p_loop->Y) {
			int half = p_loop->Y - y;

			Pack_Zap(x, y, w, half);

			y += half;
			h -= half;
			goto REDO;
		}

		if (p_loop->Y + p_loop->H < y + h) {
			int half = p_loop->Y + p_loop->H - y;

			Pack_Zap(x, y, w, half);

			y += half;
			h -= half;
			goto REDO;
		}
	}

	if (w >= h) {
		int half = w / 2;

		Pack_Zap(x, y, half, h);

		x += half;
		w -= half;
	} else {
		int half = h / 2;

		Pack_Zap(x, y, w, half);

		y += half;
		h -= half;
	}

	goto REDO;
}

/* Pack_DefragmentFully__Fv (0x8004a8cc) */
void Pack_DefragmentFully(void) {
	TSPack *p;
	TSPack *n;
	int m;

	if (Pack_pFree) {
		ZapCalls = 0;
		p = Pack_pFree;

		do {
			n = p->pNext;
			Pack_KillSlot(p);

			p = n;
		} while (p);

		Pack_pFree = 0;

		switch (Pack_Width) {
			case 320:
				Pack_Zap(320, 256, 64, 256);
				Pack_Zap(Pack_Width + 64, 256, 128, 256);
				Pack_Zap(Pack_Width + 192, 256, 512, 256);
				Pack_Zap(Pack_Width, 0, 64, 256);
				Pack_Zap(Pack_Width + 64, 0, 128, 256);
				Pack_Zap(Pack_Width + 192, 0, 512, 256);
				break;
			case 640:
				Pack_Zap(640, 256, 128, 256);
				Pack_Zap(Pack_Width + 128, 256, 256, 256);
				Pack_Zap(Pack_Width, 0, 128, 256);
				Pack_Zap(Pack_Width + 128, 0, 256, 256);
				break;
			default:
				Pack_Zap(Pack_Width, 256, 1024 - Pack_Width, 256);
				Pack_Zap(Pack_Width, 0, 1024 - Pack_Width, 256);
				break;
		}

		do {
			p = Pack_pFree;
			m = 0;

			while (p) {
				m |= Pack_Merge(p);
				p = p->pNext;
			}
		} while (m);
	}
}
