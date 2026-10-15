/* _._8CQuadBit (0x800283e0) */
/* C++ (BIT.CPP). Unlinks the quad from QuadBitList; CBit's destructor does the rest. */
#include "game/bit.h"

extern CQuadBit *QuadBitList;

CQuadBit::~CQuadBit() {
	DeleteFrom(&QuadBitList);
}
