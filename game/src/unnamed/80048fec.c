/* Db_WaitRender__Fv (0x80048fec) flags=-fstrict-aliasing */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

SDoubleBuffer *pVB;
extern unsigned char *pPoly;

void Pause__Fi(int);
void Db_CallBack(void);
int Db_OTSize(void);

/* Waits until the buffer about to be built has been drawn, then empties its ordering table and
   packet memory. */
void Db_WaitRender(void) {
	pVB = pDoubleBuffer;

	while (pVB->RenderNow)
		Pause__Fi(1);

	if (pDoubleBuffer->Rendering) {
		DrawSync(0);
		Db_CallBack();
	}

	PC_DrawClearOTR(&pDoubleBuffer->OT2D, Db_OTSize());
	pPoly = pDoubleBuffer->Polys;
}
