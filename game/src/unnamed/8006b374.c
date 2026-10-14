/* HiRes_DrawOTagAll__Fv (0x8006b374) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

extern unsigned char *pPoly;

void FlickerSync(void);
void PollHost(void);

void HiRes_DrawOTagAll(void) {
	SDoubleBuffer *a;

	PC_DrawOT(&pDoubleBuffer->OT3D[0x1fff]);

	a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	pPoly = a->Polys;
	PollHost();
}
