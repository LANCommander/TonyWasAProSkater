/* FUN_80066aa8 (0x80066aa8) */
#include "names.h"
typedef struct {
	int a, b, c, d, e;
} S;

extern int Pack_pFree;
extern int Pack_pFull;

S *DAT_800d2398[2];
int DAT_800d23a0[2];
char *DAT_800d23b0[2];
char *DAT_800d23b8[2];
int DAT_800d23a8;
int DAT_800d23ac;

S *Pack_Pack(int a, int b, int c, char *d, char *e, int f, int g, int h);
S *Mem_NewX(int a, int b, int c);

void FUN_80066aa8(void) {
	int i;
	S *s;

	for (i = 0; i < 2; i++) {
		s = Pack_Pack(-1, 160, 256, (char *)&DAT_800d23b0[i], (char *)&DAT_800d23b8[i], 16, 0,
		                 0);
		DAT_800d2398[i] = Mem_NewX(20, 1, 1);
		*DAT_800d2398[i] = *s;
		DAT_800d2398[i]->c = 0;
		DAT_800d23a0[i] = 0;
	}

	DAT_800d23a8 = Pack_pFree;
	DAT_800d23ac = Pack_pFull;
}
