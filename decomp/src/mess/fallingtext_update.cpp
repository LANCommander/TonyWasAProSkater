/* update__17FallingTextEffect (0x8001e07c) */
/* C++ (MESS.CPP). Counts every character's fall delay down by one frame. */
#include "game/mess.h"

void FallingTextEffect::update() {
	for (int i = 0; i < 32; i++)
		mWaitTime[i]--;
}
