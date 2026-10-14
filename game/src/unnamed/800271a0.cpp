/* _._9CPolyLine (0x800271a0) */
/* C++ (BIT2.CPP). Frees the segments and unlinks the line from PolyLineList; CBit's destructor does
   the rest. */
#include "game/bit.h"

extern CBit *PolyLineList;

void Mem_Delete(void *);

CPolyLine::~CPolyLine() {
	Mem_Delete(mSegs);
	DeleteFrom(&PolyLineList);
}
