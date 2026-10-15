/* Mem_Delete__FPv (0x8004d5d0) */
/* MEM: frees a block: a scratch block (heap -2) just lets go of its end of the scratch area; a
   heap block goes back on its free list. */
#include "game/mem.h"

void Mem_Delete(void *p) {
	SBlockHeader *b = (SBlockHeader *)p - 1;
	int f = b->ParentHeap;

	if (f == -2) {
		if (b == DAT_800d1b48) {
			DAT_800d1b48 = 0;
			return;
		}

		if (b == DAT_800d1b4c) {
			DAT_800d1b4c = 0;
			return;
		}
	}

	if ((uint)f < 2) {
		Used[f] -= MEM_HEADER + b->Size;
		AddToFreeList(b, f);

		if (f == 1) {
			if (((uint)Used[1]) >= CriticalBigHeapUsage)
				LowMemory = f;
			else
				LowMemory = 0;
		}
	}
}
