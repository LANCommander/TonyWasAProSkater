/* FUN_800494dc (0x800494dc) */
#include "names.h"
#ifdef PC_PORT
#include "pc_screen.h"
#include "pc_draw.h"
#endif

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

#ifndef PC_PORT /* PC: no drawing queued to reset */
	FUN_8008c1a0(1);
#endif
#ifdef PC_PORT
	PC_DrawSetEnv((void *)(pDoubleBuffer));
	PC_ScreenShow((void *)(pDoubleBuffer + 92));
#else
	FUN_8008c94c(pDoubleBuffer);
	PutDispEnv(pDoubleBuffer + 92);
#endif

	if (DAT_800d1a38 && pBMR) {
		r.x = 0;
		r.y = *(unsigned short *)(pDoubleBuffer + 32768 + 144);
		r.w = 512;
		r.h = 240;
#ifdef PC_PORT
		PC_ScreenBlit(r.x, r.y, r.w, r.h, pBMR);
#else
		LoadImage(&r, pBMR);
#endif
		DrawSync(0);
	} else {
		DAT_800d1a38 = 0;
	}

	CompactOTagR(pDoubleBuffer + 132, Db_OTSize());

	if (DAT_800d1a38)
		DrawSync(0);

#ifdef PC_PORT
	PC_DrawOT((void *)((char *)(Db_OTSize() * 4) + (int)pDoubleBuffer + 128));
#else
	DrawOTag((char *)(Db_OTSize() * 4) + (int)pDoubleBuffer + 128);
#endif
}
