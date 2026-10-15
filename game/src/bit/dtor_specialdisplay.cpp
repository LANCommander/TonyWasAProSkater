/* _._15CSpecialDisplay (0x80027f04) */
/* C++ (BIT.CPP). Unlinks the bit from SpecialDisplayList; CBit's destructor does the rest. */
#include "game/bit.h"

extern CBit *SpecialDisplayList;

CSpecialDisplay::~CSpecialDisplay() {
	DeleteFrom(&SpecialDisplayList);
}
