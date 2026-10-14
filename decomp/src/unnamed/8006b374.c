/* HiRes_DrawOTagAll__Fv (0x8006b374) */
#include "names.h"
#ifdef PC_PORT
#include "pc_draw.h"
#endif

extern char DoubleBuffer[];
extern char *pDoubleBuffer;
extern int pPoly;

int FUN_8008c1a0(int a);
void DrawOTag(char *a);
void PollHost(void);

void HiRes_DrawOTagAll(void) {
	char *a;

#ifndef PC_PORT /* PC: no drawing queued to reset */
	FUN_8008c1a0(1);
#endif
#ifdef PC_PORT
	PC_DrawOT((void *)(pDoubleBuffer + 32904));
#else
	DrawOTag(pDoubleBuffer + 32904);
#endif

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = a + 32940;

	pDoubleBuffer = a;
#ifdef PC_PORT
	/* the PS1 drew through KUSEG (bit 31 clear); on PC main RAM is only at 0x80000000 */
	pPoly = *(int *)(a + 32768 + 168) | 0x80000000;
#else
	pPoly = *(int *)(a + 32768 + 168) & 0x7fffffff;
#endif
	PollHost();
}
