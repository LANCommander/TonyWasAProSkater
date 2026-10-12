/* _._16CLinked2EndedBit (0x80029408) */
/* C++ (BIT.CPP). Unlinks the bit from Linked2EndedBitList; CFT4Bit's destructor does the rest. */
#include "game/bit.h"

CLinked2EndedBit *Linked2EndedBitList;

CLinked2EndedBit::~CLinked2EndedBit() {
	DeleteFrom(&Linked2EndedBitList);
}
