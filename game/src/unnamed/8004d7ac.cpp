/* Mem_GetHandle__FPv (0x8004d7ac) */
/* MEM: makes a handle {p, serial} for a heap block, or {0} if p isn't a live block (a serial, odd,
   in the header's first word, parent heap -2/0/1, a sane size). Mem_CheckHandle checks one.
   The `default: return h;` in the switch is what stops cse skip-blocks from reusing the first
   pNext load. */
#include "game/mem.h"

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
				h.Serial = (int)pBlock->pNext;
			}
		}
	}

	return h;
}
