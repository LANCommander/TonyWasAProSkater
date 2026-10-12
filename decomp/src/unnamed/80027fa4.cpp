/* _._15CNonRenderedBit (0x80027fa4) */
/* C++ (BIT.CPP). Unlinks the bit from NonRenderedBitList; CBit's destructor does the rest. */
#include "game/bit.h"

CNonRenderedBit *NonRenderedBitList;

CNonRenderedBit::~CNonRenderedBit() {
	DeleteFrom(&NonRenderedBitList);
}
