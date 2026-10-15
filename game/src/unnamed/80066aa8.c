/* LevelSelect_InitPackSlots__Fv (0x80066aa8) */
#include "names.h"

/* TSPack (game/m3d.h) */
typedef struct S {
	unsigned short X, Y, W, H;
	struct S *pNext;
	char Type;
	unsigned char no_bottom, bpp, aligned;
	short ReqW, ReqH;
} S;

extern S *Pack_pFree;
extern S *Pack_pFull;

extern S *DAT_800d2398[2];
extern S *DAT_800d23a0[2];
extern int DAT_800d23b0[2];
extern int DAT_800d23b8[2];
extern S *DAT_800d23a8;
extern S *DAT_800d23ac;

S *Pack_Pack(int a, int b, int c, int *d, int *e, int f, int g, unsigned h);
S *Mem_NewX(int a, int b, int c);

void LevelSelect_InitPackSlots(void) {
	int i;
	S *s;

	for (i = 0; i < 2; i++) {
		s = Pack_Pack(-1, 160, 256, &DAT_800d23b0[i], &DAT_800d23b8[i], 16, 0, 0);
		DAT_800d2398[i] = Mem_NewX(sizeof(S), 1, 1);
		*DAT_800d2398[i] = *s;
		DAT_800d2398[i]->pNext = 0;
		DAT_800d23a0[i] = 0;
	}

	DAT_800d23a8 = Pack_pFree;
	DAT_800d23ac = Pack_pFull;
}
