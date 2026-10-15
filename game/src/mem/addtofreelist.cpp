/* AddToFreeList__FP12SBlockHeaderi (0x8004ce54) */
/* MEM: puts a block back on heap idx's free list, which is sorted by address, merging it with
   the free blocks right before and after it. */
#include "game/mem.h"

void AddToFreeList(SBlockHeader *b, int idx) {
	SBlockHeader *cur = FirstFreeBlock[idx];
	SBlockHeader *prev = 0;

	while (cur < b && cur) {
		prev = cur;
		cur = cur->pNext;
	}

	if (prev) {
		if ((SBlockHeader *)((char *)prev + (prev->Size + MEM_HEADER)) == b) {
			if ((SBlockHeader *)((char *)b + (b->Size + MEM_HEADER)) == cur) {
				prev->pNext = cur->pNext;
				prev->Size += 2 * MEM_HEADER + (b->Size + cur->Size);
			} else {
				prev->Size += MEM_HEADER + b->Size;
			}

			return;
		}

		prev->pNext = b;
	} else {
		FirstFreeBlock[idx] = b;
	}

	if ((SBlockHeader *)((char *)b + (b->Size + MEM_HEADER)) == cur) {
		b->pNext = cur->pNext;
		b->Size += MEM_HEADER + cur->Size;
	} else {
		b->pNext = cur;
	}
}
