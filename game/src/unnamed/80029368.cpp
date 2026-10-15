/* _._8CFlatBit (0x80029368) */
/* C++ (BIT.CPP). Unlinks the bit from FlatBitList; CFT4Bit's destructor does the rest. */
#include "game/bit.h"

extern CFlatBit *FlatBitList;

CFlatBit::~CFlatBit() {
	DeleteFrom(&FlatBitList);
}
