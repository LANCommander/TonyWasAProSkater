/* _._9CPlatform (0x8003d7a0) */
/* C++ (PLATFORM.CPP). Leaves the environmental-object list and stops the looping sound; CBaddy's
   destructor does the rest. */
#include "game/platform.h"

extern CBody *EnvironmentalObjectList;

void SFX_Stop(uint);

CPlatform::~CPlatform() {
	DeleteFrom(&EnvironmentalObjectList);

	if (mLoopingSound) {
		SFX_Stop(mLoopingSound);

		mLoopingSound = 0;
	}
}
