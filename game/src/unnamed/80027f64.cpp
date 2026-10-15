/* __15CNonRenderedBit (0x80027f64) */
/* C++ (BIT.CPP). A bit that is moved but never drawn, linked into NonRenderedBitList. */
#include "game/bit.h"

extern CNonRenderedBit *NonRenderedBitList;

CNonRenderedBit::CNonRenderedBit() {
	AttachTo(&NonRenderedBitList);
}
