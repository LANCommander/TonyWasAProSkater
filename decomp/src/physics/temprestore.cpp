/* TempRestore__6CBruce (0x8002f118) */
/* C++ (PHYSICS.CPP). Restores the rotation matrix and position saved by TempTransform. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

void CBruce::TempRestore() {
	mRotMatrix = mTempRotMatrix;
	mPos = mTempPos;
}
