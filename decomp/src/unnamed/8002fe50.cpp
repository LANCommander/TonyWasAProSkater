/* ResetTrickBuffers__6CBruce (0x8002fe50) */
/* C++ (PHYSICS.CPP). Clears the buffered trick inputs; the older buffer only if it is more than 8
   frames old. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

extern int XFrames;

void CBruce::ResetTrickBuffers() {
	int Age = XFrames - m274;

	mTapTurn = 0;
	m370 = 0;
	m36C = 0;
	m3A0[0] = 0;
	m378[0] = 0;

	if (Age > 8) {
		m280 = 0;
		m27C = 0;
		m274 = 0;
	}

	m264 = 0;
	m260 = 0;
	m270 = 0;
	m26C = 0;
}
