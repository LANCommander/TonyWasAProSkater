/* FUN_8004cf7c (0x8004cf7c) */
/* MEM: empties both heaps, each now one free block over its whole range, and sets the big heap's
   low-memory mark at 98% of it. */
#include "game/mem.h"

void FUN_8004cf7c() {
	int i;

	for (i = 0; i < 2; i++) {
		FirstFreeBlock[i] = 0;
		Used[i] = 0;

		{
			SBlockHeader *b = (SBlockHeader *)DAT_800be09c[i].Start;

			b->Size = DAT_800be09c[i].End - (char *)b - MEM_HEADER;
			AddToFreeList(b, i);
		}
	}

	CriticalBigHeapUsage = (98 * (uint)(DAT_800be09c[1].End - DAT_800be09c[1].Start)) / 100;
}
