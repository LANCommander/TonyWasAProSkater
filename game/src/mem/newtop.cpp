/* Mem_NewTop__FUi (0x8004d3a4) flags=-fstructure-noalias */
/* C++ (MEM.CPP). Fuzzy demo twin (0.69); locals from its MAIN.SYM. Allocates from the top heap:
   best-fit over the free list (the last block big enough), splitting the block's tail off if
   there's room for another header, else taking it whole. An allocated block's pNext holds the
   allocation serial number (odd: a free block's pNext is an aligned pointer). */
#include "game/mem.h"

int LowMemory;
uint CriticalBigHeapUsage;
uint UniqueIndentifier;
int Used[2];
uint Used_4;
SBlockHeader *FirstFreeBlock[2];

void *Mem_NewTop(uint size) {
	int Heap = 1;
	int AssertOnFailure = 1;
	SBlockHeader *pBlock;
	SBlockHeader *pLast;
	SBlockHeader *pChoice;
	SBlockHeader *pChoiceLast;

	/* the serial an allocated block keeps in its header's pNext: odd, so it can't be the free
	   list's (aligned) pointer (Mem_GetHandle) */
	UniqueIndentifier = ((UniqueIndentifier + 2) & 0x7fffffff) | 1;
	size = MEM_ROUND(size);
	pBlock = FirstFreeBlock[Heap];
	pLast = 0;
	pChoice = pBlock;
	pChoiceLast = 0;

	while (pBlock) {
		if (pBlock->Size >= size) {
			pChoice = pBlock;
			pChoiceLast = pLast;
		}

		pLast = pBlock;
		pBlock = pLast->pNext;
	}

	if (pChoice->Size < size) {
		if (AssertOnFailure) {
			if (Heap == 1)
				Bang(128, 0, 0);
			else if (Heap == 0)
				Bang(0, 0, 128);
		}

		return 0;
	}

	if (size + MEM_HEADER < pChoice->Size) {
		uint t;
		uint *pMem = (uint *)((char *)pChoice + (t = pChoice->Size + MEM_HEADER) - size);

		pChoice->Size = pChoice->Size - size - MEM_HEADER;
		pBlock = (SBlockHeader *)pMem - 1;
		pBlock->Size = size;
		pBlock->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
		pBlock->ParentHeap = 1;
		Used[Heap] += pBlock->Size + MEM_HEADER;

		if (Heap == 1) {
			if (Used_4 >= CriticalBigHeapUsage)
				LowMemory = Heap;
			else
				LowMemory = 0;
		}

		return pBlock + 1;
	}

	pLast = pChoiceLast;

	if (pLast)
		pLast->pNext = pChoice->pNext;
	else
		FirstFreeBlock[Heap] = pChoice->pNext;

	pChoice->ParentHeap = 1;
	pChoice->pNext = (SBlockHeader *)(uintptr_t)UniqueIndentifier;
	Used[Heap] += pChoice->Size + MEM_HEADER;

	if (Heap == 1) {
		if (Used_4 >= CriticalBigHeapUsage)
			LowMemory = Heap;
		else
			LowMemory = 0;
	}

	return pChoice + 1;
}
