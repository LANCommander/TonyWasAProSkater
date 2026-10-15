/* Trig_RemoveDualBuffer__Fv (0x8004634c) */
#include "names.h"

extern void *pPSXDualBuffer;

void Mem_Delete(void *p);

void Trig_RemoveDualBuffer(void) {
	if (pPSXDualBuffer)
		Mem_Delete(pPSXDualBuffer);

	pPSXDualBuffer = 0;
}
