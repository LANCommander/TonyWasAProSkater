/* Pack_Init__Fv (0x80049780) */
#include "names.h"

extern struct TSPack *Pack_pFree, *Pack_pFull;
extern int TotSlots, Pack_Width, LastPackInitType;

void *Pack_NewFree(int, int, int, int);

void Pack_Init(void) {
	TotSlots = 0;
	Pack_pFree = 0;
	Pack_pFull = 0;
	Pack_Width = 512;
	Pack_NewFree(512, 256, 512, 256);
	Pack_NewFree(512, 0, 512, 256);

	LastPackInitType = 0;
}
