/* HiRes_DrawOTagNoSync__Fv (0x8006b2f4) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

extern unsigned char *pPoly;

void FlickerSync(void);
void PollHost(void);

void HiRes_DrawOTagNoSync(void) {
	SDoubleBuffer *a;

	PC_DrawOT(&pDoubleBuffer->OT3D[0x1000]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}
