/* __16CLinked2EndedBit (0x800293c8) */
/* C++ (BIT.CPP). A sprite stretched between two end points, linked into Linked2EndedBitList. */
#include "game/bit.h"

extern CLinked2EndedBit *Linked2EndedBitList;

CLinked2EndedBit::CLinked2EndedBit() {
	AttachTo(&Linked2EndedBitList);
}
