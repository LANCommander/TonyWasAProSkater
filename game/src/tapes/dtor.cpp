/* _._5CTape (0x80073228) */
/* C++ (TAPES.CPP). Unlinks the tape from the power-up list and deletes its glow; CBody's
   destructor does the rest. */
#include "game/tapes.h"

extern CBody *PowerUpList;

CTape::~CTape() {
	DeleteFrom(&PowerUpList);
	DeleteStuff();
}
