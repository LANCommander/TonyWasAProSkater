/* _._7CRibbon (0x8002a018) */
/* C++ (BIT.CPP). Deletes the ribbon's segments, then frees the segment and point arrays;
   CNonRenderedBit's destructor does the rest. */
#include "game/bit.h"

void Mem_Delete(void *);

CRibbon::~CRibbon() {
	for (int i = 0; i < mNumBits; i++)
		delete mBits[i];

	Mem_Delete(mBits);
	Mem_Delete(mPoints);
}
