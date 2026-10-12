/* Pack_Pack__FiiiPiT3iiUi (0x8004982c) flags=-fstructure-noalias */
/* C++ (PACK.CPP). Demo twin (0.97 by opcode; THPS1 also records the requested size in the block);
   locals from its MAIN.SYM. Finds VRAM space for a W x H texture of the given depth: aligned
   requests first try an exact fit, else carve an aligned slot out of a bigger (or a fresh 256x256)
   allocation and free the leftovers; otherwise pick the smallest free block that fits (from the
   left, then from the right of a block, then any block with an aligned start), split it and
   return the new full block. AlignedX/AlignedW are one pair for the third search and the split
   after it, so AlignedW lives across the Pack_NewFree call in s0, as retail's does. */
#include "game/m3d.h"

TSPack *Pack_pFree;
int Pack_NoBottom;
int Pack_NoRight;

TSPack *Pack_NewFree(int, int, int, int);
void Pack_Merge(TSPack *);
void Pack_Remove(TSPack **, TSPack *);
extern "C" TSPack *FUN_800496b0(int, int, int, int);

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

		pBest = FUN_800496b0(OX + OW - W, OY, W, H);

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

		pBest = FUN_800496b0(OX, OY, W, H);

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
