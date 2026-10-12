/* init__17FallingTextEffect (0x8001e014) */
/* C++ (MESS.CPP). Gives each character a random fall delay of 15..44 frames and resets the
   effect's state words at 0x40/0x46 to 280 and 240. */
#include "game/mess.h"

int Redbook_Rnd(int);

void FallingTextEffect::init() {
	for (int i = 0; i < 32; i++)
		mWaitTime[i] = Redbook_Rnd(30) + 15;

	m40 = 280;
	m46 = 240;
}
