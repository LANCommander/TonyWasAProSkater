/* HiRes_DrawOTag__Fv (0x8006b26c) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

extern unsigned char *pPoly;

void FlickerSync(void);
void PollHost(void);
/* set by front-end screens that redraw part of the background themselves (platform/pc) */
extern void (*pc_hires_before_draw)(void);

void HiRes_DrawOTag(void) {
	SDoubleBuffer *a;

	FlickerSync();
	/* after the wait for the next field, so the redraw and this frame's primitives are shown together */
	if (pc_hires_before_draw)
		pc_hires_before_draw();
	PC_DrawOT(&pDoubleBuffer->OT3D[0x1000]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}
