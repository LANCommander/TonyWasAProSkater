/* Pack_Init640x480__Fv (0x800497d4) */
#include "names.h"

extern struct TSPack *Pack_pFree, *Pack_pFull;
extern int TotSlots, Pack_Width, LastPackInitType;

void *Pack_NewFree(int, int, int, int);

void Pack_Init640x480(void) {
	TotSlots = 0;
	Pack_Width = 640;
	Pack_pFree = 0;
	Pack_pFull = 0;
	Pack_NewFree(640, 256, 384, 256);
	Pack_NewFree(640, 0, 384, 256);

	LastPackInitType = 1;
}
