/* Pack_KillSlot__FP6TSPack (0x800495f4) */
#include "names.h"

extern int TotSlots;

void Mem_Delete(void *);

void Pack_KillSlot(void *p) {
	Mem_Delete(p);

	TotSlots = TotSlots - 1;
}
