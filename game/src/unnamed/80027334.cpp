/* _._10CGPolyLine (0x80027334) */
/* C++ (BIT2.CPP). Frees the segments and unlinks the line from GPolyLineList; CBit's destructor does
   the rest. */
#include "game/bit.h"

extern CBit *GPolyLineList;

void Mem_Delete(void *);

CGPolyLine::~CGPolyLine() {
	Mem_Delete(mSegs);
	DeleteFrom(&GPolyLineList);
}
