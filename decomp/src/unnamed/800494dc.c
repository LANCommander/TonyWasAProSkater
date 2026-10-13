/* FUN_800494dc (0x800494dc) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} R;

extern int pBMR;

char *pDoubleBuffer;
int DAT_800d1a38;

void FUN_8008c1a0(int a);
void FUN_8008c94c(char *a);
int PutDispEnv(char *a);
void LoadImage(R *r, int a);
void DrawSync(int a);
int Db_OTSize(void);
int CompactOTagR(char *a, int b);
void DrawOTag(char *a);

void FUN_800494dc(void) {
	R r;

	FUN_8008c1a0(1);
	FUN_8008c94c(pDoubleBuffer);
	PutDispEnv(pDoubleBuffer + 92);

	if (DAT_800d1a38 && pBMR) {
		r.x = 0;
		r.y = *(unsigned short *)(pDoubleBuffer + 32768 + 144);
		r.w = 512;
		r.h = 240;
		LoadImage(&r, pBMR);
		DrawSync(0);
	} else {
		DAT_800d1a38 = 0;
	}

	CompactOTagR(pDoubleBuffer + 132, Db_OTSize());

	if (DAT_800d1a38)
		DrawSync(0);

	DrawOTag((char *)(Db_OTSize() * 4) + (int)pDoubleBuffer + 128);
}
