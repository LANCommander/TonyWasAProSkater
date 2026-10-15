/* __15CSpecialDisplay (0x80027ec4) */
/* C++ (BIT.CPP). A bit with its own display routine: links itself into SpecialDisplayList. */
#include "game/bit.h"

extern CBit *SpecialDisplayList;

CSpecialDisplay::CSpecialDisplay() {
	AttachTo(&SpecialDisplayList);
}
