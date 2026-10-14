/* _._6CGLine (0x800275ec) */
/* C++ (BIT.CPP). Unlinks the line from GLineList; CBit's destructor does the rest. */
#include "game/bit.h"

extern CBit *GLineList;

CGLine::~CGLine() {
	DeleteFrom(&GLineList);
}
