/* Shake__7CCameraRC7CVector10EShakeType (0x800789d8) */
/* C++ (CAMERA.CPP). Starts a camera shake of the given size, then passes the shake on to every baddy
   and environmental object. */
#include "game/camera.h"

/* set by CAMERA's static initialiser (0x8007d6e4) */
extern CSVector BigShakeAmp, MediumShakeAmp, SmallShakeAmp, LandShakeSpeed;
extern CFriction LandShakeDecay;

extern CBody *BaddyList;
extern CBody *EnvironmentalObjectList;

void CCamera::Shake(const CVector &pos, EShakeType type) {
	switch (type) {
		case CAMERASHAKE_BIG:
			mShakeAmplitude = BigShakeAmp;
			goto land;
		case CAMERASHAKE_MEDIUM:
			mShakeAmplitude = MediumShakeAmp;
			goto land;
		case CAMERASHAKE_SMALL:
			mShakeAmplitude = SmallShakeAmp;
		land:
			mShakeDecay = LandShakeDecay;
			mShakeSpeed = LandShakeSpeed;
			break;
	}

	SendShake(pos, BaddyList, type);
	SendShake(pos, EnvironmentalObjectList, type);
}
