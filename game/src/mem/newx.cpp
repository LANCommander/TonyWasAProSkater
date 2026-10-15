/* Mem_NewX__FUiii (0x8004d068) flags=-fstructure-noalias */
/* C++ (MEM.CPP). Demo twin (retail adds the scratch heap); locals and blocks from its MAIN.SYM.
   Heap 0 is the top heap (Mem_NewTop). Heap -2 is a two-ended scratch area between
   DAT_800d1b40 and DAT_800d1b44: one block from the bottom, one from the top. The other heaps are
   first-fit free lists: a big enough block is split (its tail goes back on the free list) or
   taken whole. An allocated block's pNext holds the allocation serial number (odd: a free
   block's pNext is an aligned pointer). */
#include "game/mem.h"

extern int LowMemory;
extern uint CriticalBigHeapUsage;
extern uint UniqueIndentifier;
extern int Used[2];
extern SBlockHeader *FirstFreeBlock[2];
extern char *DAT_800d1b40;
extern char *DAT_800d1b44;
extern SBlockHeader *DAT_800d1b48;
extern SBlockHeader *DAT_800d1b4c;

void *Mem_NewX(uint size, int Heap, int AssertOnFailure) {
	SBlockHeader *pBlock;
	SBlockHeader *pLast;
	char buf[40];

	if (!Heap)
		return Mem_NewTop(size);

	/* the serial an allocated block keeps in its header's pNext: odd, so it can't be the free
	   list's (aligned) pointer (Mem_GetHandle) */
	UniqueIndentifier = ((UniqueIndentifier + 2) & 0x7fffffff) | 1;
	size = MEM_ROUND(size);

	if (Heap == -2) {
		int avail;

		if (!DAT_800d1b48) {
			if (DAT_800d1b4c)
				avail = (char *)DAT_800d1b4c - DAT_800d1b40 - MEM_HEADER;
			else
				avail = DAT_800d1b44 - DAT_800d1b40 - 2 * MEM_HEADER;

			if (avail < size || avail < 0) {
				if (AssertOnFailure)
					Bang(128, 128, 0);

				return 0;
			}

			DAT_800d1b48 = (SBlockHeader *)DAT_800d1b40;
			MEM_INFO(DAT_800d1b48) = (size << 4) | 14;
			DAT_800d1b48->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
			return DAT_800d1b48 + 1;
		} else {
			int total = DAT_800d1b44 - DAT_800d1b40;
			int used = DAT_800d1b48->Size + 2 * MEM_HEADER;

			avail = total - used;

			if (avail < size) {
				if (AssertOnFailure)
					Bang(128, 128, 0);

				return 0;
			}

			DAT_800d1b4c = (SBlockHeader *)(DAT_800d1b44 - size) - 1;
			DAT_800d1b4c->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
			MEM_INFO(DAT_800d1b4c) = (size << 4) | 14;
			return DAT_800d1b4c + 1;
		}
	}

	pBlock = FirstFreeBlock[Heap];
	pLast = 0;

	while (pBlock->Size < size) {
		if (!pBlock->pNext)
			break;

		pLast = pBlock;
		pBlock = pBlock->pNext;
	}

	if (pBlock->Size < size) {
		if (AssertOnFailure) {
			if (Heap == 1)
				Bang(128, 0, 0);
			else if (Heap == 0)
				Bang(0, 0, 128);
		}

		return 0;
	}

	if (pBlock->Size > size + MEM_HEADER) {
		SBlockHeader *pLeftOver = (SBlockHeader *)((char *)pBlock + (size + MEM_HEADER));

		pLeftOver->Size = pBlock->Size - size - MEM_HEADER;

		if (pLast)
			pLast->pNext = pBlock->pNext;
		else
			FirstFreeBlock[Heap] = pBlock->pNext;

		MEM_INFO(pBlock) = (Heap & 15) | (size << 4);
		pBlock->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
		Used[Heap] += pBlock->Size + MEM_HEADER;
		AddToFreeList(pLeftOver, Heap);

		if (Heap == 1) {
			if (((uint)Used[1]) >= CriticalBigHeapUsage)
				LowMemory = Heap;
			else
				LowMemory = 0;
		}

		return pBlock + 1;
	}

	if (pLast)
		pLast->pNext = pBlock->pNext;
	else
		FirstFreeBlock[Heap] = pBlock->pNext;

	pBlock->ParentHeap = Heap;
	pBlock->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
	Used[Heap] += pBlock->Size + MEM_HEADER;

	if (Heap == 1) {
		if (((uint)Used[1]) >= CriticalBigHeapUsage)
			LowMemory = Heap;
		else
			LowMemory = 0;
	}

	return pBlock + 1;
}
