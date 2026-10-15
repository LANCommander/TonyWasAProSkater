/* _._4CBit (0x80027e80) */
/* C++ (BIT.CPP). Counts the bit out of BitCount. */
#include "game/bit.h"

extern int BitCount;

CBit::~CBit() {
	BitCount--;
}
