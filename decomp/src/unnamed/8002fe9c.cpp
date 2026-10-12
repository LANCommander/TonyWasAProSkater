/* Bail__6CBrucei13ELimitBailVel (0x8002fe9c) */
/* C++ (PHYSICS.CPP). Starts a bail: ends the combo, goes into the air without the board, runs the
   bail animation (harder above a speed), and plays a skater-specific (or random) bail sound. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/career.h"

struct Message;

extern SStats Stats[];

int Mips_CVecDotProd(CVector *, CVector *);
void SFX_PlayX(uint, short, int);
int Redbook_Rnd(int);

extern "C" int FUN_8008b5dc(int);

void CBruce::Bail(int BailNumber, ELimitBailVel) {
	mBailNumber = BailNumber;
	m724 = 0;
	mBails++;
	Trick_Bail();

	m71C = 0;
	mInAir = 1;
	mJumpPending = 0;
	mJumpStrength = 0;
	ResetTrickBuffers();

	mDoingATrick = 0;
	mStickToRail = 0;
	StateTransition(PHYSICS_IN_AIR, 0x412);
	BoardOff();

	mAnimSpeed = 0x10000;
	Debug_RunAnim2(0x36, 0, 0x1d, 0x416);

	if (0x2d000 < (FUN_8008b5dc(Mips_CVecDotProd(&mVel, &mVel)) << 6))
		m24C = 1;
	else
		m24C = 6;

	FlipToVelocity();

	if (Stats[mSkaterIndex].ComboID == 0x9000 || Stats[mSkaterIndex].ComboID == 0xb000) {
		SFX_PlayX(8, 0x2000, 0);
		SFX_PlayX(2, 0x2000, 0);
	} else {
		if (Redbook_Rnd(2))
			SFX_PlayX(9, 0x2000, 0);
		else
			SFX_PlayX(10, 0x2000, 0);
	}

	m6DC = 0;
}
