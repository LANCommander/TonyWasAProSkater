/* Pack_GetSlot__Fv (0x800495c4) */
#include "names.h"

extern int TotSlots;

void *Mem_NewX(int a, int b, int c);

/* a new TSPack (its callers use what Mem_NewX returns, which retail's v0 still held) */
void *Pack_GetSlot(void) {
	void *p = Mem_NewX(20, 0, 1);

	TotSlots = TotSlots + 1;
	return p;
}
