/* __4CBit (0x80027e44) */
/* C++ (BIT.CPP). Base particle: friction shift 1 on each axis; counts live bits. */
#include "game/bit.h"

int BitCount;

CBit::CBit() : mFric(1, 1, 1) {
	BitCount++;
}
