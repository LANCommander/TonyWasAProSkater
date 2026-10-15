/* __7CCameraP5CBodyUs (0x800785f0) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Fuzzy demo twin (0.97 by opcode); members from its MAIN.SYM, laid out by THPS1
   offsets (CCamera members = demo + 0x2c). Most members are set in the initialiser list. */
#include "game/camera.h"
#include "game/bruce.h"

extern SControl Pad[2];
extern SPresetCamera Presets[5];

extern int camDist;
extern int camPitch;
extern int camYOffset;
extern int Camera_Preset;
extern int NumCameras;
extern CBody *CameraList;

CCamera::CCamera(CBody *tripod, ushort mike_index)
    : mMikeIndex(mike_index), mTripod(tripod), mCalcTripod(1), new_focus(0), mFocus(tripod),
      mCalcFocus(1), mZoom(192), mOnRail(0), mPitchDamper(0), mTripodPosYDamper(0),
      mCollisionRayLR(-1), mCollisionRayBack(-1), mCollisionAngLR(512), mCollisionAngBack(784),
      mCollisionBack(1384), mCameraMode(CAMERAMODE_NORMAL), mCameraModeIncDir(ASCENDING),
      mBossCamSpinRate(15), mTimer(0), testQuat(mTimer), mColCheckIndex(0),
      mColCheckCycleCounter(0), mDownDist(0), mSpeedUpIntro(0), mStoppedExtraYRot(0) {
	loose_cam_timer = 0;

	if (((CBruce *)mTripod)->mpPad == &Pad[0])
		mMikeIndex = 0;
	else if (((CBruce *)mTripod)->mpPad == &Pad[1])
		mMikeIndex = 1;

	vFront = CVector(0, 0, 4096);
	big_air = 0;
	in_pool_timer = 1000;
	extra_YRot = 0;
	mTripodPos = mTripod->mPos;
	mTripodTime = 0;
	mFocusPos = mFocus->mPos;
	mFocusTime = 0;
	mType = 99;
	Camera_Preset = 2;

	camDist = Presets[Camera_Preset].Dist;
	camPitch = Presets[Camera_Preset].Pitch;
	camYOffset = Presets[Camera_Preset].YOffset;

	mFlags = 1;
	LoadIntoMikeCamera();
	AttachTo(&CameraList);

	NumCameras++;
	mRMinor = 0;
	mCBodyFlags &= ~2;
}
