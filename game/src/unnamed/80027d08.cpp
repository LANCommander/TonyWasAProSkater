/* __nw__4CBitUi (0x80027d08) */
/* C++ (BIT.CPP). Allocates a bit from the main heap and zeroes it (both arms of the
   UseCacheForBits test allocate the same way in retail). */
#include "game/bit.h"

int UseCacheForBits;

void *Mem_NewX(unsigned int, int, int);

void *CBit::operator new(__SIZE_TYPE__ Size) throw() {
	int *p;

	if (UseCacheForBits)
		p = (int *)Mem_NewX(Size, 0, 1);
	else
		p = (int *)Mem_NewX(Size, 0, 1);

	uint NumWords = (Size + 3) >> 2;
	int *q = p;

	for (uint i = 0; i < NumWords; i++)
		*q++ = 0;

	return p;
}
