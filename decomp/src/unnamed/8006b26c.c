/* HiRes_DrawOTag__Fv (0x8006b26c) */
#include "names.h"

extern char DoubleBuffer[];
extern char *pDoubleBuffer;
extern int pPoly;

void FlickerSync(void);
int FUN_8008c1a0(int a);
void DrawOTag(char *a);
void PollHost(void);
#ifdef PC_PORT
/* set by front-end screens that redraw part of the background themselves (platform/pc) */
extern void (*pc_hires_before_draw)(void);
#endif

void HiRes_DrawOTag(void) {
	char *a;

	FlickerSync();
#ifdef PC_PORT
	/* after the wait for the next field, so the redraw and this frame's primitives are shown together */
	if (pc_hires_before_draw)
		pc_hires_before_draw();
#endif
	FUN_8008c1a0(1);
	DrawOTag(pDoubleBuffer + 16524);

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
