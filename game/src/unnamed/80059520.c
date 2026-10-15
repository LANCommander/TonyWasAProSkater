/* Front_InitSkaterPack__Fi (0x80059520) flags=-fstructure-noalias */
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

extern S *DAT_800d1c84;
extern S *DAT_800d1c80;
extern S *DAT_800d1c88;
extern S *DAT_800d1c8c;
extern S *DAT_800d1c90;
extern char DAT_800d1c94[8];
extern char DAT_800d1c98[8];

S *Pack_Pack(int a, int b, int c, char *d, char *e, int f, int g, int h);
S *Mem_NewX(int a, int b, int c);

void Front_InitSkaterPack(int m) {
	S *s;
	int t1, t2;
	int w = 256;

	if (m == 1)
		w = 128;

	s = Pack_Pack(-1, w, 256, DAT_800d1c94, DAT_800d1c98, 16, 0, 0);
	DAT_800d1c84 = Mem_NewX(sizeof(S), 1, 1);
	*DAT_800d1c84 = *s;
	DAT_800d1c80 = Mem_NewX(sizeof(S), 1, 1);
	*DAT_800d1c80 = *s;
	DAT_800d1c88 = 0;
	DAT_800d1c84->pNext = 0;
	DAT_800d1c8c = Pack_pFree;
	DAT_800d1c90 = Pack_pFull;
}
