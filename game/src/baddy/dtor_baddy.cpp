/* _._6CBaddy (0x8003faa8) */
/* C++ (BADDY.CPP). Stops the baddy's looping sound; CSuper's destructor does the rest. */
#include "game/baddy.h"

extern int NumBaddies;

void SFX_Stop(uint);

CBaddy::~CBaddy() {
	NumBaddies--;

	if (mLoopingSound)
		SFX_Stop(mLoopingSound);
}
