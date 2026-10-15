/* Mem_Shrink__FPvUi (0x8004d6a0) */
/* MEM: shrinks a block to n bytes, giving the rest back to its heap as a block of its own (if
   there's room for a header). */
#include "game/mem.h"

void Mem_Shrink(void *p, uint n) {
	SBlockHeader *b = (SBlockHeader *)p - 1;
	uint l, n8;
	int f = b->ParentHeap;
	SBlockHeader *r;

	if (f == -2) {
		if (b == DAT_800d1b48) {
			b->Size = n;
			return;
		}

		if (b == DAT_800d1b4c)
			return;
	}

	l = b->Size;

	if (n < l - MEM_HEADER) {
		r = (SBlockHeader *)((char *)p + n);
		n8 = n + MEM_HEADER;
		r->Size = l - n8;
		Used[f] -= b->Size - n;
		b->Size = n;
		AddToFreeList(r, f);

		if (f == 1) {
			if (((uint)Used[1]) >= CriticalBigHeapUsage)
				LowMemory = f;
			else
				LowMemory = 0;
		}
	}
}
