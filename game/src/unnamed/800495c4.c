/* Pack_GetSlot__Fv (0x800495c4) */
#include "names.h"

int TotSlots;

void *Mem_NewX(int a, int b, int c);

void Pack_GetSlot(void) {
	Mem_NewX(20, 0, 1);

	TotSlots = TotSlots + 1;
}
