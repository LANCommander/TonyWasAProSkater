/* TrickOb_TrickedOnItem__FP4ItemPvi (0x8004f000) */
#include "names.h"

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
typedef struct N {
	char p0[4];
	int f4;
	unsigned short s8;
	unsigned char cA, cB;
	unsigned short sC;
	char p1[2];
	struct N *f10;
	void *f14;
} N;

typedef struct B {
	unsigned short f0;
	char p0[0x20 - 2];
	int f20;
	char p1[0x1d4 - 0x24];
	struct B *f1d4;
	char p2[0x1f0 - 0x1d8];
	int f1f0;
	int f1f4;
	char p3[0x924 - 0x1f8];
	int f924;
} B;

N *TrickObList;

extern B *GSkater;
extern int GGame;

int TrickPlayer, TrickScore;

int FUN_8006ef00(B *);
int FUN_8004eed8(unsigned short);
int SFX_PlayX(int, short, int);
void FUN_8006eb18(int *);
void FUN_80068f40(int, int);
unsigned short *Trig_GetLinksPointer(unsigned short);
void Trig_SendPulse(unsigned short *);

int TrickOb_TrickedOnItem(B *a, B *b, int c) {
	unsigned char s5;
	int s3, s1, v, ret;
	N *s0;

	s5 = 2;

	if (b == GSkater)
		s5 = 1;

	v = FUN_8006ef00(a);
	s3 = 0;
	s0 = TrickObList;

	while (s0 && s0->f4 != v) {
		s0 = s0->f10;
		s3++;
	}

	if (!s0)
		return 1;

	if (c == -1) {
		ret = FUN_8004eed8(s0->s8);

		if (!ret)
			goto out;
	}

	if (s0->cA) {
		if (GGame != 8)
			return 3;

		if (s0->cB == s5)
			return 3;
	}

	s1 = 0;

	if (FUN_8004eed8(s0->s8)) {
		if (GGame == 1) {
			SFX_PlayX(20, 0x2000, 0);

			a->f20 = 0x202020;
			s1 = 1;
			a->f0 |= 0x400;
			goto L122;
		}
	}

	if (GGame == 8 && c >= s0->sC) {
		s0->sC = c;
		a->f20 = 0x202020;
		a->f0 |= 0x400;
		b->f1f0++;
		SFX_PlayX(24, 0x2000, 0);

		if (s0->cA) {
			b->f1d4->f1f0--;
			b->f1f4++;
			FUN_8006eb18(&b->f924);
			SFX_PlayX(25, 0x2000, 0);
		}

		s0->cB = s5;
		s1 = 1;
	}
L122:
	if (s1) {
		TrickPlayer = s5;
		TrickScore = c;
		FUN_80068f40(s5, s3);
		Trig_SendPulse(Trig_GetLinksPointer(s0->s8));

		s0->cA = 1;
		s0->f14 = a;
	}

	return 3;
out:
	return ret;
}
