/* __6CBaddy (0x8003fa34) flags=-fstructure-noalias */
/* C++ (BADDY.CPP). Demo twin (statement order from its line table). Counts baddies and sets the
   script/collision defaults. */
#include "game/baddy.h"

int NumBaddies;

CBaddy::CBaddy() {
	NumBaddies++;
	mNode = 0xffff;
	mRMinor = 128;
	mWaitTimer = 0;
	mVerticalCompliance = 32;
	mPushVal = 64;
	mpWeaponAttributes = 0;
	mLoopingSound = 0;
	mLoopingSoundUpdate = -1;
}
