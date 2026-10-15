/* __nw__5CItemUi (0x800434d4) */
/* C++ (OB.CPP). CItem's operator new: allocates from heap 0 (asserting on failure) and zeroes the
   block a word at a time. */
#include "game/ob.h"

void *Mem_NewX(uint, int, int);

void *CItem::operator new(__SIZE_TYPE__ size) throw() {
	void *p = Mem_NewX(size, 0, 1);
	int *q = (int *)p;
	uint words = (size + 3) >> 2;

	for (uint i = 0; i < words; i++)
		*q++ = 0;

	return p;
}
