/* Mem_CheckHandle__FP10SMemHandle (0x8004d83c) */
/* MEM: the block a handle (Mem_GetHandle) refers to, if it is still the same allocation (its
   header still holds the serial); else clears the handle and returns 0. */
#include "game/mem.h"

void *Mem_CheckHandle(SMemHandle *h) {
	void *p = h->p;

	if (!p)
		return 0;

	if ((int)(uintptr_t)((SBlockHeader *)p - 1)->pNext == h->Serial)
		return p;

	h->p = 0;
	return 0;
}
