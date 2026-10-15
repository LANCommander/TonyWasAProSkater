/* TrickOb_TrickedOnItem__FP4ItemPvi (0x8004f000) */
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/panel.h"
#include "game/sfx.h"
#include "game/trickob.h"

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
extern CTrickOb *TrickObList;

extern CBruce *GSkater;
extern int GGame;

extern int TrickPlayer, TrickScore;

extern "C" {
	int FUN_8006ef00(Item *);
	int FUN_8004eed8(ushort);
	void FUN_8006eb18(CPanelData *);
	void FUN_80068f40(int, int);
}
ushort *Trig_GetLinksPointer(int);
void Trig_SendPulse(ushort *);

int TrickOb_TrickedOnItem(Item *a, void *pb, int c) {
	CBruce *b = (CBruce *)pb;
	uchar s5;
	int s3, s1, v, ret;
	CTrickOb *s0;

	s5 = 2;

	if (b == GSkater)
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
			SFX_PlayX(20, 0x2000, 0);

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
		SFX_PlayX(24, 0x2000, 0);

		if (s0->mState) {
			b->mpOtherSkater->mGraffitiCount--;
			b->mGraffitiSteals++;
			FUN_8006eb18(&b->mPanel);
			SFX_PlayX(25, 0x2000, 0);
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
