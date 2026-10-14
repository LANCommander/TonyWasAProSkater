/* __nw__6CClassUi (0x800241dc) */
/* C++ (OB.CPP). CClass's operator new: allocates from heap 0 (asserting on failure) and zeroes the
   block a word at a time. */
#include "game/class.h"

void *Mem_NewX(uint, int, int);

void *CClass::operator new(uint size) throw() {
	void *p = Mem_NewX(size, 0, 1);
	int *q = (int *)p;

	for (uint i = 0; i < (size + 3) >> 2; i++)
		*q++ = 0;

	return p;
}
