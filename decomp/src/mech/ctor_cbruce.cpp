/* __6CBruce (0x80041e5c) flags=-fstructure-noalias */
/* C++ (MECH.CPP). Demo twin; statement order from its line table. The skater constructor: an
   inline CPanelData ctor (retail has it inline, demo out of line), then the per-level/game-mode
   defaults, the two kick speeds from stat 1, the identity-ish rotation, list attach and
   shadow setup. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/camera.h"
#include "game/career.h"
#include "game/pad.h"

extern SStats Stats[];
extern SLevel Levels[];
extern int GLevel;
extern int GGame;
extern int Cheat_StatsOverride;
extern int Cheat_Fire;
extern int GNumberOfPlayers;
extern CCamera *CameraList;
extern int DAT_800a8118[];
extern SControl Pad[];

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

void *Mem_New(uint, int, int);

int NumMechs;
CBody *MechList;

CBruce::CBruce() : mPanel(this) {
	int i;

	mAnimSpeed = 0x10000;

	if (Levels[GLevel].isCompetition && GGame == 1)
		mTime = 3630;
	else if (GGame != 7)
		mTime = 7230;
	else
		mTime = 630;

	mGravity.Set(0, 1000, 0);

	mFlags |= 0x200;
	mScale.Set(4096, 4096, 4096);

	mMaxSpeed = ((GetStat(1) + 39) << 12) * 3 * 15 / 28;
	mKickSpeed = ((GetStat(1) + 17) << 12) * 3 * 15 / 28;
	m340 = 30;
	m344 = 30;
	m6DC = 0;
	mStickToRail = 0;
	m70C = 0;
	mGrindSnd = 0;
	m710 = -1;
	mRollSnd = 0;
	mPhysicsState = PHYSICS_IN_AIR;
	mOldPhysicsState = PHYSICS_IN_AIR;
	m6A8 = 0;
	mManIsCrouched = 0;
	m740 = 0;
	mJumpPending = 0;
	mDoingATrick = 0;
	m750 = 0;
	m424 = 0;
	mExtraFlags |= 1;

	for (i = 0; i < 80; i++)
		m428[i] = 0;

	for (i = 0; i < 80; i++)
		m568[i] = 0;

	m764 = 0;
	m768 = 0;
	mFallLine.Set(0, 0, 0);

	mFallLineTurn = 0;
	mPolyNormal.vx = 0;
	mPolyNormal.vy = -4096;
	mPolyNormal.vz = 0;
	mPitchAngle = 0;
	mLeanAngle = 0;
	mAnimLeanAngle = 0;
	m840 = 0;
	mRotMatrix.m[0][0] = -4096;
	mRotMatrix.m[1][0] = 0;
	mRotMatrix.m[2][0] = 0;
	mRotMatrix.m[0][1] = 0;
	mRotMatrix.m[1][1] = -4096;
	mRotMatrix.m[2][1] = 0;
	mRotMatrix.m[0][2] = 0;
	mRotMatrix.m[1][2] = 0;
	mRotMatrix.m[2][2] = -4096;
	FRUVectorsFromRotMatrix();

	mOldRotMatrix = mRotMatrix;
	int x = mRotMatrix.m[0][1];
	int y = mRotMatrix.m[1][1];
	int z = mRotMatrix.m[2][1];

	mSameNormal = 0;
	mOldNormal.vx = x;
	mOldNormal.vy = y;
	mOldNormal.vz = z;
	m98 = 2;
	mNormal.vx = x;
	mNormal.vy = y;
	mNormal.vz = z;
	AttachTo(&MechList);

	NumMechs++;
	mCBodyFlags &= ~2;

	if (NumMechs == 1)
		mpPad = Pad;
	else
		mpPad = &Pad[1];

	mAngFric.Set(5, 1, 5);

	if (GNumberOfPlayers == 1) {
		mShadowLights = 1;
		mpShadowSource = (SVECTOR *)Mem_New(32, 0, 1);
		mpShadowRGB = (CVECTOR *)Mem_New(16, 0, 1);
		((short *)mpShadowSource)[0] = 0;
		((short *)mpShadowSource)[1] = -4096;
		((short *)mpShadowSource)[2] = 0;
		((uchar *)mpShadowRGB)[0] = 128;
		((uchar *)mpShadowRGB)[1] = 128;
		((uchar *)mpShadowRGB)[2] = 128;
	} else {
		ShadowOn();
		UpdateShadow();
	}

	CameraList->mCameraMode = CAMERAMODE_NORMAL;
	mpLight = (SLight *)DAT_800a8118;
}
