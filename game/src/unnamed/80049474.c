/* FUN_80049474 (0x80049474) */
#include "names.h"
#include "game/dbuffer.h"
#include "pc_draw.h"

extern unsigned char *pPoly;

int Db_OTSize(void);

/* Swaps to the other buffer, with its ordering table cleared and its packet memory empty. */
void FUN_80049474(void) {
	SDoubleBuffer *a = DoubleBuffer;

	if (pDoubleBuffer == a)
		a = &DoubleBuffer[1];

	pDoubleBuffer = a;
	PC_DrawClearOTR(&pDoubleBuffer->OT2D, Db_OTSize());
	pPoly = pDoubleBuffer->Polys;
}
