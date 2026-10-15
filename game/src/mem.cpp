#include "game/mem.h"
#include "game/globals.h"
#include "pc_runtime.h"

/* Bang__FUcUcUc (0x8004ce28) */
/* The game's fatal-error handler: on the PS1 it fills the screen with a colour and spins. The port
   reports which heap ran out (each heap's free list) and stops (PC_Bang). */
void Bang(uchar r, uchar g, uchar b) {
	int heap;

	PC_Log("Bang(%d, %d, %d): the game hit a fatal error (out of memory if from Mem_New*)", r, g, b);

	for (heap = 0; heap < 2; heap++) {
		SBlockHeader *pBlock;
		uint total = 0, n = 0, big = 0;

		for (pBlock = FirstFreeBlock[heap]; pBlock && n < 100000; pBlock = pBlock->pNext, n++) {
			total += pBlock->Size;

			if (pBlock->Size > big)
				big = pBlock->Size;
		}

		PC_Log("  heap %d: %u free blocks, %u bytes free, largest %u; used %d", heap, n, total,
		       big, Used[heap]);
	}

	PC_Bang(r, g, b);
}

/* AddToFreeList__FP12SBlockHeaderi (0x8004ce54) */
/* MEM: puts a block back on heap idx's free list, which is sorted by address, merging it with
   the free blocks right before and after it. */
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

/* FUN_8004cf7c (0x8004cf7c) */
/* MEM: empties both heaps, each now one free block over its whole range, and sets the big heap's
   low-memory mark at 98% of it. */
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

/* Mem_NewX__FUiii (0x8004d068) */
/* Demo twin (retail adds the scratch heap); locals and blocks from its MAIN.SYM.
   Heap 0 is the top heap (Mem_NewTop). Heap -2 is a two-ended scratch area between
   DAT_800d1b40 and DAT_800d1b44: one block from the bottom, one from the top. The other heaps are
   first-fit free lists: a big enough block is split (its tail goes back on the free list) or
   taken whole. An allocated block's pNext holds the allocation serial number (odd: a free
   block's pNext is an aligned pointer). */
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

/* Mem_NewTop__FUi (0x8004d3a4) */
/* Fuzzy demo twin (0.69); locals from its MAIN.SYM. Allocates from the top heap:
   best-fit over the free list (the last block big enough), splitting the block's tail off if
   there's room for another header, else taking it whole. An allocated block's pNext holds the
   allocation serial number (odd: a free block's pNext is an aligned pointer). */
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
			if (((uint)Used[1]) >= CriticalBigHeapUsage)
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
		if (((uint)Used[1]) >= CriticalBigHeapUsage)
			LowMemory = Heap;
		else
			LowMemory = 0;
	}

	return pChoice + 1;
}

/* Mem_Delete__FPv (0x8004d5d0) */
/* MEM: frees a block: a scratch block (heap -2) just lets go of its end of the scratch area; a
   heap block goes back on its free list. */
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

/* Mem_Shrink__FPvUi (0x8004d6a0) */
/* MEM: shrinks a block to n bytes, giving the rest back to its heap as a block of its own (if
   there's room for a header). */
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

/* Mem_GetHandle__FPv (0x8004d7ac) */
/* MEM: makes a handle {p, serial} for a heap block, or {0} if p isn't a live block (a serial, odd,
   in the header's first word, parent heap -2/0/1, a sane size). Mem_CheckHandle checks one.
   The `default: return h;` in the switch is what stops cse skip-blocks from reusing the first
   pNext load. */
SMemHandle Mem_GetHandle(void *p) {
	SMemHandle h;

	h.p = 0;

	if (p) {
		SBlockHeader *pBlock = (SBlockHeader *)p - 1;

		if ((uintptr_t)pBlock->pNext & 1) {
			switch (pBlock->ParentHeap) {
				case 0:
				case 1:
				case -2:
					break;
				default:
					return h;
			}

			if (pBlock->Size - 4 <= 0x1ffffc) {
				h.p = p;
				h.Serial = (int)(uintptr_t)pBlock->pNext;
			}
		}
	}

	return h;
}

/* Mem_CheckHandle__FP10SMemHandle (0x8004d83c) */
/* MEM: the block a handle (Mem_GetHandle) refers to, if it is still the same allocation (its
   header still holds the serial); else clears the handle and returns 0. */
void *Mem_CheckHandle(SMemHandle *h) {
	void *p = h->p;

	if (!p)
		return 0;

	if ((int)(uintptr_t)((SBlockHeader *)p - 1)->pNext == h->Serial)
		return p;

	h->p = 0;
	return 0;
}

/* Mem_GetFree__Fc (0x8004d878) */
/* MEM: the bytes heap i has left. */
int Mem_GetFree(char i) {
	return DAT_800be09c[(uchar)i].End - DAT_800be09c[(uchar)i].Start - Used[(uchar)i];
}

/* Mem_Copy__FPvT0i (0x8004d8b4) */
void Mem_Copy(int *d, int *s, int n) {
	int w = n >> 2;
	char *dc, *sc;

	n &= 3;

	while (w > 0) {
		*d++ = *s++;
		w--;
	}

	dc = (char *)d;
	sc = (char *)s;

	while (n > 0) {
		*dc++ = *sc++;
		n--;
	}
}
