/* _._8CGenPoly (0x8002870c) */
/* C++ (BIT.CPP). Unlinks the bit from GenPolyList; CBit's destructor does the rest. */
#include "game/bit.h"

extern char GenPolyList[8];

CGenPoly::~CGenPoly() {
	DeleteFrom(GenPolyList);
}
