/* TRICKOB.CPP: the trick objects, CTrickOb, scoring/graffiti targets on TrickObList
   (declarations in game/trickob.h). */
#include "game/trickob.h"
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/panel.h"
#include "game/sfx.h"
#include "game/globals.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/video.h"

/* the egcs vtable as TrickObList's deleting-destructor calls use it: slot 0's this-offset and
   function */
struct STrickObVtable {
	short off0, i0; /* entry 0 (unused) */
	void *fn0;
	short off;
	short pad2;
	void (*fn)(char *, int);
};
extern char __vt_8CTrickOb[];
extern char DAT_800a1330[];
void __dl__6CClassPv(void *p);
/* the last tagging skater (uchar) and its trick score (ushort) */
extern int TrickScore;
extern "C" {
	int FUN_8006ef00(Item *);
	int FUN_8004eed8(ushort);
	void FUN_8006eb18(CPanelData *);
}

/* TrickOb_DeleteAll__Fv (0x8004ee78) */
void TrickOb_DeleteAll(void) {
	CTrickOb *p = TrickObList;
	CTrickOb *n;
	struct STrickObVtable *vt;

	while (p) {
		n = p->mpNext;

		if (p) {
			vt = *(struct STrickObVtable **)p;
			vt->fn((char *)p + vt->off, 3);
		}

		p = n;
	}
}

/* FUN_8004eed8 (0x8004eed8) */
int FUN_8004eed8(unsigned short i) {
	return *OffsetList[i] == 14;
}

/* __8CTrickObUs (0x8004ef08) */
CTrickOb::CTrickOb(unsigned short Node) {
	mpNext = TrickObList;
	TrickObList = this;
	mpItem = 0;
	mNode = Node;
	mState = 0;

	{
		unsigned short *pNode = OffsetList[Node];

		pNode += pNode[1] + 2;

		if ((intptr_t)pNode & 3)
			pNode++;

		mChecksum = *(unsigned int *)pNode;
	}
}

/* FUN_8004ef80 (0x8004ef80) */
void FUN_8004ef80(CTrickOb *p, int f) {
	CTrickOb *prev = 0;
	CTrickOb *c = TrickObList;

	*(char **)p = __vt_8CTrickOb;

	while (c != p) {
		prev = c;
		c = c->mpNext;
	}

	if (prev)
		prev->mpNext = p->mpNext;
	else
		TrickObList = p->mpNext;

	*(char **)p = DAT_800a1330;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* TrickOb_TrickedOnItem__FP4ItemPvi (0x8004f000) */
/* EXP module. 146/147: the only residue is the zero return after FUN_8004eed8: cse follows the
   single-use `goto out` and folds `return ret` to `v0 = 0`, which reorg copies into the `beq`
   slot (retail: nop). Giving `out` a second user (`if(!s0){ret=1;goto out;}`, retail's
   `j; li v0,1` shape) fixes that but then reorg fills the final `beq s1,zero` slot from the
   target (`v0 = 3`) instead of the fall-through (142/147). S29: the two are coupled. Every
   variant without the folded `v0 = 0` block flips that slot: more `goto out` users, direct
   returns, ret/callee types, C++ `bool`, and -fno-cse-follow-jumps (142). Likely cause: once
   the `j; out:` block is gone, reorg's find_basic_block can't place the beq target, treats
   all regs as live, and can't take `move a0,s5` from the fall-through. No per-TU flag helps
   (match_sym ignores CC1_OPTS; sweep with match_div). FUN_8006eb18 takes one argument;
   SFX_PlayX returns int. */
int TrickOb_TrickedOnItem(Item *a, void *pb, int c) {
	CBruce *b = (CBruce *)pb;
	uchar s5;
	int s3, s1, v, ret;
	CTrickOb *s0;

	s5 = 2;

	if (b == GSkater[0])
		s5 = 1;

	v = FUN_8006ef00(a);
	s3 = 0;
	s0 = TrickObList;

	while (s0 && s0->mChecksum != v) {
		s0 = s0->mpNext;
		s3++;
	}

	if (!s0)
		return 1;

	if (c == -1) {
		ret = FUN_8004eed8(s0->mNode);

		if (!ret)
			goto out;
	}

	if (s0->mState) {
		if (GGame != 8)
			return 3;

		if (s0->mPlayer == s5)
			return 3;
	}

	s1 = 0;

	if (FUN_8004eed8(s0->mNode)) {
		if (GGame == 1) {
			SFX_PlayX(SFX_LETTER, 0x2000, 0);

			a->RGB = 0x202020;
			s1 = 1;
			a->Flags |= 0x400;
			goto L122;
		}
	}

	if (GGame == 8 && c >= s0->mScore) {
		s0->mScore = c;
		a->RGB = 0x202020;
		a->Flags |= 0x400;
		b->mGraffitiCount++;
		SFX_PlayX(SFX_GRAFFITI_TAG, 0x2000, 0);

		if (s0->mState) {
			b->mpOtherSkater->mGraffitiCount--;
			b->mGraffitiSteals++;
			FUN_8006eb18(&b->mPanel);
			SFX_PlayX(SFX_GRAFFITI_STEAL, 0x2000, 0);
		}

		s0->mPlayer = s5;
		s1 = 1;
	}
L122:
	if (s1) {
		TrickPlayer = s5;
		TrickScore = c;
		FUN_80068f40(s5, s3);
		Trig_SendPulse(Trig_GetLinksPointer(s0->mNode));

		s0->mState = 1;
		s0->mpItem = a;
	}

	return 3;
out:
	return ret;
}

/* TrickOb_DarkenNode__Fi (0x8004f24c) */
void TrickOb_DarkenNode(int id) {
	CTrickOb *n;

	if (GGame == 8) {
		for (n = TrickObList; n; n = n->mpNext) {
			if (n->mNode == id) {
				Item *it;

				if (n->mState && GGame != 8)
					return;

				it = Spool_FindEnviroItem(n->mChecksum);
				n->mScore = *(ushort *)&TrickScore;
				n->mpItem = it;
				n->mPlayer = *(uchar *)&TrickPlayer;
				it->RGB = 0x202020;
				it->Flags |= 0x400;
				n->mState = 1;
				return;
			}
		}
	}
}

/* TrickOb_GetItemType__FP4Item (0x8004f318) */
int TrickOb_GetItemType(Item *o) {
	unsigned int c = FUN_8006ef00(o);
	CTrickOb *p;

	for (p = TrickObList; p; p = p->mpNext)
		if (p->mChecksum == c)
			break;

	return p ? 2 : 1;
}

/* TrickOb_Update__Fv (0x8004f374) */
void TrickOb_Update(void) {
	CTrickOb *pTrickOb;

	for (pTrickOb = TrickObList; pTrickOb; pTrickOb = pTrickOb->mpNext) {
		if (pTrickOb->mState) {
			Item *pItem = pTrickOb->mpItem;

			if (pItem) {
				/* read every pass (its own TU saw GGame as `const int`, which kept the load in
				   the loop) */
				if (*(volatile int *)&GGame == 8) {
					int Red;
					uint t = Xblanks << 2;

					if (pTrickOb->mScore <= 2000)
						Red = t % ((((pTrickOb->mScore << 6) / 2000) + 1) * 2);
					else
						Red = t % 130;

					if (pTrickOb->mScore <= 2000) {
						if ((((pTrickOb->mScore << 6) / 2000) + 1) < Red)
							goto flip;
					} else {
						if (Red >= 66) {
						flip:
							if (pTrickOb->mScore <= 2000)
								Red = ((((pTrickOb->mScore << 6) / 2000) + 1) * 2) - Red;
							else
								Red = 130 - Red;
						}
					}

					if (pTrickOb->mPlayer == 1)
						pItem->RGB = (pItem->RGB & 0xff00ffff) | ((Red + 64) << 16);
					else
						pItem->RGB = (pItem->RGB & 0xffffff00) | (Red + 64);
				} else {
					int Red = (Xblanks >> 1) & 63;

					if (Red > 32)
						Red = 64 - Red;

					pItem->RGB = (pItem->RGB & 0xffffff00) | (Red + 32);
				}
			}
		}
	}
}

/* FUN_8004f544 (0x8004f544) */
void FUN_8004f544(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1330;

	if (f & 1)
		__dl__6CClassPv(p);
}
