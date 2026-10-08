/* Pack_KillSlot__FP6TSPack (0x800495f4) */
#include "names.h"
int TotSlots;

void Mem_Delete(void);

void Pack_KillSlot(void) {
	Mem_Delete();

	TotSlots = TotSlots - 1;
}
