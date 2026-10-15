/* CAMERA.CPP: the skater camera: CCamera (a CBody: tripod/focus tracking, collision, shakes and
   the normal, competition-intro and no-big-air modes), its CamPos debug positions and the
   module's static initialiser and inline destructor copies (declarations in game/camera.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/camera.h"
#include "game/bruce.h"
#include "game/colij.h"
#include "game/m3d.h"
#include "game/vector.h"
#include "game/m3dasm.h"
#include "game/padbuttons.h"
#include "game/pad.h"
#include "game/career.h"
#include "game/video.h"
#include "game/bit.h"
#include "game/competition.h"
#include "game/font.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/psyq.h"
#include "game/quat.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/m3dzone.h"

extern SPresetCamera Presets[5];
extern int Camera_Preset;
extern int NumCameras;
/* set by CAMERA's static initialiser (0x8007d6e4) */
extern CSVector BigShakeAmp, MediumShakeAmp, SmallShakeAmp, LandShakeSpeed;
extern CFriction LandShakeDecay;
extern CVector Camera_CamOffset;
extern int Camera_CamDist;
CQuat Quat_FromXRot(short);
CQuat Quat_FromYRot(short);
#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
extern MATRIX tm;
extern int Camera_CamPitch;
extern unsigned short DAT_800d2068, DAT_800d206a;
#define SIN(a) (rcossin_tbl[(a) & 4095].s)
extern int Mick_CameraTweak;
extern int DAT_800d2084;
extern int DAT_800d2088;
extern int DAT_800d208c;
extern int DAT_800d2090;
extern int old_camYOffset;
extern int old_camDist;
extern int old_camPitch;
CQuat Quat_FromZRot(short);
extern char __vt_6CamPos[];
extern char DAT_800a35c4[];
void __dl__6CClassPv(void *p);
extern CQuat UpDownPosition_8, testquat2, testquat3;
extern CVector DemoFocus_8, DAT_800d3a04, DAT_800d3a14, DAT_800d3a24, DAT_800d3a34, DAT_800d3a44,
    DAT_800d3a54, DAT_800d3a64, DAT_800d3a74, DAT_800d3a84, DAT_800d3a94, FlyFocus, DAT_800d3ab4;
extern char DAT_800a35b4[];
extern char DAT_800a3594[];

/* __7CCameraP5CBodyUs (0x800785f0) */
/* Fuzzy demo twin (0.97 by opcode); members from its MAIN.SYM, laid out by THPS1
   offsets (CCamera members = demo + 0x2c). Most members are set in the initialiser list. */
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

/* _._7CCamera (0x800788b8) */
/* Unlinks the camera from CameraList; the member quaternions/CamPos and CBody's
   destructor do the rest. */
CCamera::~CCamera() {
	DeleteFrom(&CameraList);

	NumCameras--;
}

/* SetCollisionRayLR__7CCamerai (0x80078988) */
/* Sets the left/right collision ray length. */
void CCamera::SetCollisionRayLR(int v) {
	mCollisionRayLR = v;
}

/* SetCollisionRayBack__7CCamerai (0x80078990) */
/* Sets the backward collision ray length. */
void CCamera::SetCollisionRayBack(int v) {
	mCollisionRayBack = v;
}

/* SetCollisionAngLR__7CCameras (0x80078998) */
/* Sets the left/right collision ray angle. */
void CCamera::SetCollisionAngLR(short v) {
	mCollisionAngLR = v;
}

/* SetCollisionAngBack__7CCameras (0x800789a0) */
/* Sets the backward collision ray angle. */
void CCamera::SetCollisionAngBack(short v) {
	mCollisionAngBack = v;
}

/* SendShake__FRC7CVectorP5CBody10EShakeType (0x800789a8) */
void SendShake(const CVector &pos, CBody *b, EShakeType type) {
	while (b) {
		b->mInputFlags |= 2;
		b = (CBody *)b->mNextItem;
	}
}

/* Shake__7CCameraRC7CVector10EShakeType (0x800789d8) */
/* Starts a camera shake of the given size, then passes the shake on to every baddy
   and environmental object. */
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

/* SetComplexMode__7CCameraUs (0x80078afc) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetComplexMode(ushort mode) {}

/* LoadIntoMikeCamera__7CCamera (0x80078b04) */
void CCamera::LoadIntoMikeCamera() {
	VECTOR v;

	if (((CBruce *)mTripod)->mPhysicsState == 5) {
		MikeCamera[Camera_CurrentMikeCamera].Position.vx = (mTripodPos.vx + old_offset.vx) >> 12;
		MikeCamera[Camera_CurrentMikeCamera].Position.vy = (mTripodPos.vy + old_offset.vy) >> 12;
		MikeCamera[Camera_CurrentMikeCamera].Position.vz = (mTripodPos.vz + old_offset.vz) >> 12;
		mO = old_orientation;
	} else {
		MikeCamera[Camera_CurrentMikeCamera].Position.vx = (mPos.vx + real_cam_offset.vx) >> 12;
		MikeCamera[Camera_CurrentMikeCamera].Position.vy = (mPos.vy + real_cam_offset.vy) >> 12;
		MikeCamera[Camera_CurrentMikeCamera].Position.vz = (mPos.vz + real_cam_offset.vz) >> 12;
		old_orientation = mO;
		old_offset.vx = (MikeCamera[Camera_CurrentMikeCamera].Position.vx << 12) - mTripodPos.vx;
		old_offset.vy = (MikeCamera[Camera_CurrentMikeCamera].Position.vy << 12) - mTripodPos.vy;
		old_offset.vz = (MikeCamera[Camera_CurrentMikeCamera].Position.vz << 12) - mTripodPos.vz;
	}

	MikeCamera[Camera_CurrentMikeCamera].Focus.vx = (mFocusPos.vx + Camera_CamOffset.vx) >> 12;
	MikeCamera[Camera_CurrentMikeCamera].Focus.vy = (mFocusPos.vy + Camera_CamOffset.vy) >> 12;
	MikeCamera[Camera_CurrentMikeCamera].Focus.vz = (mFocusPos.vz + Camera_CamOffset.vz) >> 12;
	QToM(mO, MikeCamera[Camera_CurrentMikeCamera].Transform);
	TransMatrix(&MikeCamera[Camera_CurrentMikeCamera].Transform,
	            &MikeCamera[Camera_CurrentMikeCamera].Position);
}

/* ResetOrientation__7CCamera (0x80078d94) */
void CCamera::ResetOrientation() {
	VECTOR vtemp1, vtemp2;
	CBruce *pBruce;

	pBruce = (CBruce *)mTripod;
	vtemp1.vx = 0;
	vtemp1.vy = 0;
	vtemp1.vz = -4096;
	ApplyMatrixLV(&((CBruce *)mTripod)->mRotMatrix, &vtemp1, &vtemp2);

	vFront.vx = vtemp2.vx;
	vFront.vy = vtemp2.vy;
	vFront.vz = vtemp2.vz;
	mVFront = mOldVFront = vFront << 12;
	mOldTripodPos = mTripodPos = mTripod->mPos;
	Camera_CurrentMikeCamera = mMikeIndex;
	CM_Normal();

	mStoppedO = mPrevO = mO = mDesiredO;
	mStoppedOValid = 1;
	LoadIntoMikeCamera();
}

static inline int AvgS(short a, short b) {
	return (a + b) >> 1;
}

/* ReactToCollision__7CCamera (0x80078f3c) */
/* Demo twin (opcode-identical but for a few `lui`s); locals and frame from its
   MAIN.SYM. CCamera members are the demo's + 0x2c. */
void CCamera::ReactToCollision() {
	SLineInfo line_info;
	MATRIX m;
	VECTOR vLeft;
	VECTOR vRight;
	VECTOR vBack;
	VECTOR vTemp;
	CVector vVel;
	CVector vStick;
	CVector vColCheck;
	VECTOR vOffset;

	if (GamePaused || GameFrozen)
		return;

	if (mTimer < 13)
		return;

	QToM(mDesiredO, m);

	if (++mColCheckIndex >= 3) {
		mColCheckIndex = 0;
		mColCheckCycleCounter++;
	}

	if (mColCheckIndex == 2) {
		mPrevColAngBuffer[2] = mColAngBuffer[2];
		line_info.StartCoords.vx = mPos.vx;
		line_info.StartCoords.vy = mPos.vy;
		line_info.StartCoords.vz = mPos.vz;
		line_info.EndCoords.vx = mPos.vx;
		line_info.EndCoords.vy = mPos.vy;
		line_info.EndCoords.vz = mPos.vz;
		line_info.EndCoords.vy += 0xbb8000;
		M3dColij_InitLineInfo(&line_info);

		line_info.RecordTriggerZoneHits = 0;
		M3dZone_LineToItemNotSCached(&line_info, 1);

		if (line_info.pItem) {
			mDownDist = line_info.Distance;

			if (line_info.Distance < 120) {
				int ratio;
				int ang;

				ratio = ((120 - line_info.Distance) << 12) / 120;
				ang = (ratio * ratio) >> 15;
				mColAngBuffer[2] = ang;
			} else {
				mColAngBuffer[2] = 0;
			}
		} else {
			if (((CBruce *)mTripod)->mPhysicsState == 1)
				mColAngBuffer[2] = 512;
			else
				mColAngBuffer[2] = mPrevColAngBuffer[2];

			mDownDist = -1;
		}

		if (((CBruce *)mTripod)->mPhysicsState == 2) {
			vTemp.vx = 0;
			vTemp.vy = Camera_CamDist << 13;
			vTemp.vz = 0;
			ApplyMatrixLV(&m, &vTemp, &vBack);

			line_info.StartCoords.vx = mPos.vx;
			line_info.StartCoords.vy = mPos.vy;
			line_info.StartCoords.vz = mPos.vz;
			line_info.EndCoords.vx = mPos.vx + vBack.vx;
			line_info.EndCoords.vy = mPos.vy + vBack.vy;
			line_info.EndCoords.vz = mPos.vz + vBack.vz;
			M3dColij_InitLineInfo(&line_info);

			line_info.RecordTriggerZoneHits = 0;
			CameraCollisionCheck = 1;
			M3dZone_LineToItemNotSCached(&line_info, 1);

			CameraCollisionCheck = 0;
			mColAngBuffer[2] = 0;

			if (line_info.pItem) {
				int ratio;
				int ang;

				if (!line_info.Length)
					line_info.Length = 1;

				ratio = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
				ratio = (ratio * ratio) >> 12;
				ang = 300;
				ang *= ratio;
				ang >>= 12;
				mColAngBuffer[2] += ang;
				mDownDist = line_info.Distance;
			} else {
				mDownDist = Camera_CamDist;
			}
		}
	}

	do {
		vTemp.vz = 0;
		vTemp.vy = 0;
		vTemp.vx = -(Camera_CamDist << 13);
	} while (0);

	ApplyMatrixLV(&m, &vTemp, &vLeft);

	vRight.vx = -vLeft.vx;
	vRight.vy = -vLeft.vy;
	vRight.vz = -vLeft.vz;

	if (mCollisionRayLR) {
		if (mColCheckIndex == 0) {
			mPrevColAngBuffer[0] = mColAngBuffer[0];

			if (mColCheckCycleCounter & 1) {
				line_info.StartCoords.vx = mPos.vx;
				line_info.StartCoords.vy = mPos.vy;
				line_info.StartCoords.vz = mPos.vz;
				line_info.EndCoords.vx = mPos.vx + vLeft.vx;
				line_info.EndCoords.vy = mPos.vy + vLeft.vy;
				line_info.EndCoords.vz = mPos.vz + vLeft.vz;
			} else {
				line_info.StartCoords.vx = mTripodPos.vx;
				line_info.StartCoords.vy = mTripodPos.vy;
				line_info.StartCoords.vz = mTripodPos.vz;
				line_info.EndCoords.vx = ((mPos.vx + mTripodPos.vx) >> 1) + vLeft.vx;
				line_info.EndCoords.vy = ((mPos.vy + mTripodPos.vy) >> 1) + vLeft.vy;
				line_info.EndCoords.vz = ((mPos.vz + mTripodPos.vz) >> 1) + vLeft.vz;
			}

			M3dColij_InitLineInfo(&line_info);

			line_info.RecordTriggerZoneHits = 0;
			CameraCollisionCheck = 1;
			M3dZone_LineToItemNotSCached(&line_info, 1);

			CameraCollisionCheck = 0;
			mColAngBuffer[0] = 0;

			if (line_info.pItem) {
				int ratio;
				int ang;

				if (!line_info.Length)
					line_info.Length = 1;

				ratio = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
				ratio = (ratio * ratio) >> 12;
				ang = (mCollisionAngLR * ratio) >> 12;
				mColAngBuffer[0] = ang;
			}
		}

		if (mColCheckIndex == 1) {
			mPrevColAngBuffer[1] = mColAngBuffer[1];

			if (mColCheckCycleCounter & 1) {
				line_info.StartCoords.vx = mPos.vx;
				line_info.StartCoords.vy = mPos.vy;
				line_info.StartCoords.vz = mPos.vz;
				line_info.EndCoords.vx = mPos.vx + vRight.vx;
				line_info.EndCoords.vy = mPos.vy + vRight.vy;
				line_info.EndCoords.vz = mPos.vz + vRight.vz;
			} else {
				line_info.StartCoords.vx = mTripodPos.vx;
				line_info.StartCoords.vy = mTripodPos.vy;
				line_info.StartCoords.vz = mTripodPos.vz;
				line_info.EndCoords.vx = ((mPos.vx + mTripodPos.vx) >> 1) + vRight.vx;
				line_info.EndCoords.vy = ((mPos.vy + mTripodPos.vy) >> 1) + vRight.vy;
				line_info.EndCoords.vz = ((mPos.vz + mTripodPos.vz) >> 1) + vRight.vz;
			}

			M3dColij_InitLineInfo(&line_info);

			line_info.RecordTriggerZoneHits = 0;
			CameraCollisionCheck = 1;
			M3dZone_LineToItemNotSCached(&line_info, 1);

			CameraCollisionCheck = 0;
			mColAngBuffer[1] = 0;

			if (line_info.pItem) {
				int ratio;
				int ang;

				if (!line_info.Length)
					line_info.Length = 1;

				ratio = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
				ratio = (ratio * ratio) >> 12;
				ang = (mCollisionAngLR * ratio) >> 12;
				mColAngBuffer[1] = -ang;
			}
		}
	}

	vTemp.vx = 0;
	vTemp.vy = 0;
	vTemp.vz = -(Camera_CamDist << 12);
	ApplyMatrixLV(&m, &vTemp, &vBack);

	vTemp.vx = 0x64000;
	vTemp.vy = 0;
	vTemp.vz = 0;
	ApplyMatrixLV(&m, &vTemp, &vOffset);

	if (big_air_timer <= 0 && ((CBruce *)mTripod)->mPhysicsState != 2 && mCollisionBack) {
		uint ratioL;
		uint ratioR = 0;
		int ang;

		mPrevColAngBuffer[3] = mColAngBuffer[3];
		line_info.StartCoords.vx = mTripodPos.vx + (vOffset.vx >> 1);
		line_info.StartCoords.vy = mTripodPos.vy + (vOffset.vy >> 1);
		line_info.StartCoords.vz = mTripodPos.vz + (vOffset.vz >> 1);
		line_info.EndCoords.vx = mTripodPos.vx - vOffset.vx + vBack.vx * 2;
		line_info.EndCoords.vy = mTripodPos.vy - vOffset.vy + vBack.vy * 2;
		line_info.EndCoords.vz = mTripodPos.vz - vOffset.vz + vBack.vz * 2;
		M3dColij_InitLineInfo(&line_info);

		line_info.RecordTriggerZoneHits = 0;
		CameraCollisionCheck = 1;
		M3dZone_LineToItemNotSCached(&line_info, 1);

		CameraCollisionCheck = 0;
		ratioL = 0;

		if (line_info.pItem) {
			if (!line_info.Length)
				line_info.Length = 1;

			ratioL = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
			ratioL *= ratioL;
			ratioL >>= 12;
		}

		line_info.StartCoords.vx = mTripodPos.vx - (vOffset.vx >> 1);
		line_info.StartCoords.vy = mTripodPos.vy - (vOffset.vy >> 1);
		line_info.StartCoords.vz = mTripodPos.vz - (vOffset.vz >> 1);
		line_info.EndCoords.vx = mTripodPos.vx + vOffset.vx + vBack.vx * 2;
		line_info.EndCoords.vy = mTripodPos.vy + vOffset.vy + vBack.vy * 2;
		line_info.EndCoords.vz = mTripodPos.vz + vOffset.vz + vBack.vz * 2;
		M3dColij_InitLineInfo(&line_info);

		line_info.RecordTriggerZoneHits = 0;
		CameraCollisionCheck = 1;
		M3dZone_LineToItemNotSCached(&line_info, 1);

		CameraCollisionCheck = 0;

		if (line_info.pItem) {
			if (!line_info.Length)
				line_info.Length = 1;

			ratioR = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
			ratioR *= ratioR;
			ratioR >>= 12;
		}

		ang = 1384;

		if (((CBruce *)mTripod)->mPhysicsState == 4) {
			ratioR = 0;
			ratioL = 0;
		}

		mColAngBuffer[3] = 0;

		if (ratioR || ratioL) {
			if (ColAng(1) && ColAng(0) < -ColAng(1)) {
				ang *= ratioR;
				ang >>= 12;
				mColAngBuffer[3] = -ang;
			}

			if (ColAng(0) && -ColAng(1) < ColAng(0)) {
				ang *= ratioL;
				ang >>= 12;
				mColAngBuffer[3] = ang;
			}
		}
	}

	if (big_air_timer > 0 || ((CBruce *)mTripod)->mPhysicsState == 2) {
		mColAngBuffer[3] = 0;
		mPrevColAngBuffer[3] = 0;
	}

	Utils_CalcUnit(vVel, mVel);

	if (mColCheckIndex == 4) {
		mPrevColAngBuffer[4] = mColAngBuffer[4];
		vVel *= Camera_CamDist;
		line_info.EndCoords.vx = mPos.vx + vVel.vx;
		line_info.EndCoords.vy = mPos.vy + vVel.vy;
		line_info.EndCoords.vz = mPos.vz + vVel.vz;
		line_info.StartCoords.vx = mPos.vx;
		line_info.StartCoords.vy = mPos.vy;
		line_info.StartCoords.vz = mPos.vz;
		M3dColij_InitLineInfo(&line_info);

		line_info.RecordTriggerZoneHits = 0;
		CameraCollisionCheck = 1;
		M3dZone_LineToItemNotSCached(&line_info, 1);

		CameraCollisionCheck = 0;
		Utils_CalcUnit(vVel, mVel);
		Utils_CalcUnit(vStick, mTripod->mPos - mPos);

		mColAngBuffer[4] = 0;

		if (line_info.pItem) {
			int ratio;
			int ang;

			if (!line_info.Length)
				line_info.Length = 1;

			ratio = ((line_info.Length - line_info.Distance) << 12) / line_info.Length;
			ang = (ratio * ratio) >> 16;
			mColAngBuffer[4] = ang;
		}
	}

	if (ColAng(2))
		mDesiredO = mDesiredO * Quat_FromXRot(ColAng(2));

	if (ColAng(0))
		mDesiredO = mDesiredO * Quat_FromYRot(ColAng(0));

	if (ColAng(1))
		mDesiredO = mDesiredO * Quat_FromYRot(ColAng(1));

	if (mColAngBuffer[4]) {
		Utils_CalcUnit(vStick, mTripod->mPos - mPos);
		geom_cross(&vVel, &vStick, &vColCheck, 0);

		vColCheck.vx *= mColAngBuffer[4];
		vColCheck.vy *= mColAngBuffer[4];
		vColCheck.vz *= mColAngBuffer[4];
		mDesiredO *= QFromV(vColCheck);
	}

	if (ColAng(3))
		mDesiredO *= Quat_FromYRot(ColAng(3));

	if (mStoppedOValid) {
		VECTOR v_temp;
		VECTOR v_temp2;
		CVector vworlddown(0, 4096, 0);
		CVector vdown;
		CVector vstop;
		MATRIX mstop;
		int dot_stop;

		v_temp.vx = 0;
		v_temp.vy = 4096;
		v_temp.vz = 0;
		ApplyMatrixLV(&m, &v_temp, &v_temp2);

		vdown.vx = v_temp2.vx;
		vdown.vy = v_temp2.vy;
		vdown.vz = v_temp2.vz;
		QToM(mStoppedO, mstop);

		v_temp.vx = 0;
		v_temp.vy = 0;
		v_temp.vz = 4096;
		ApplyMatrixLV(&mstop, &v_temp, &v_temp2);

		vstop.vx = v_temp2.vx;
		vstop.vy = v_temp2.vy;
		vstop.vz = v_temp2.vz;
		dot_stop = ABS(Mips_CVecDotProd(&vdown, &vstop));
		ABS(Mips_CVecDotProd(&vworlddown, &vstop));

		if (mDownDist < 0 || dot_stop >= 1901)
			mStoppedOValid = 0;
	}

	if (mSpeed < 4 || mTripodPos == mOldTripodPos || ((CBruce *)mTripod)->mInAir) {
		if (!mStoppedOValid && mDownDist >= 0) {
			Quat_Slerp(mO, mDesiredO, 1536, mStoppedO);

			mStoppedOValid = 1;
		}

		if (!GState && mStoppedOValid) {
			if (!ColAng(0) && !ColAng(1) && !ColAng(3))
				Quat_Slerp(mO, mDesiredO, 1511, mStoppedO);

			mDesiredO = mStoppedO;

			if (ColAng(1) && !ColAng(0) && !ColAng(3))
				mDesiredO = mDesiredO * Quat_FromYRot(ColAng(1));
			else if (!AvgS(mPrevColAngBuffer[1], mColAngBuffer[1]) && ColAng(0) && !ColAng(3))
				mDesiredO = mDesiredO * Quat_FromYRot(ColAng(0));
		}
	} else {
		mStoppedOValid = 0;
		mStoppedExtraYRot = 0;
	}

	if (mStoppedExtraYRot)
		mDesiredO *= Quat_FromYRot(mStoppedExtraYRot);
}

/* MoveToDesiredPos__7CCameraUi (0x8007a494) */
/* Fuzzy demo twin (0.89 by opcode); locals from its MAIN.SYM. Moves the camera
   each frame. With no tripod it builds a look-at basis from the focus direction and slerps to it.
   Otherwise it averages the tripod's speed into mph and a speed lag, tilts down if a ray behind
   the camera is clear, then either does big-air handling (lifting the offset by hang time) or
   normal tilt/loosen handling, slerps mO toward mDesiredO, and places the camera.
   THPS1 drops the demo's |N.V| test (always the second basis) and the gp camera-offset copy.
   fold() moves a constant onto the *other* factor of a product: `temptime * 8000 * big_air_mph`
   shifts big_air_mph and multiplies by temptime, as retail does. Locals are declared where
   first used so their stack slots reuse the earlier blocks' slots. */
void CCamera::MoveToDesiredPos(uint normal_slerp) {
	mPrevO = mO;

	if (!mTripod) {
		CVector U;
		CVector V;
		CVector N;

		Utils_CalcUnit(N, mFocusPos - mTripodPos);

		V.vx = 0;
		V.vy = 4096;
		V.vz = 0;
		geom_cross(&V, &N, &U, 0);
		U.vx >>= 12;
		U.vy >>= 12;
		U.vz >>= 12;
		geom_cross(&N, &U, &V, 0);
		V.vx >>= 12;
		V.vy >>= 12;
		V.vz >>= 12;
		tm.m[0][0] = U.vx;
		tm.m[1][0] = U.vy;
		tm.m[2][0] = U.vz;
		tm.m[0][1] = V.vx;
		tm.m[1][1] = V.vy;
		tm.m[2][1] = V.vz;
		tm.m[0][2] = N.vx;
		tm.m[1][2] = N.vy;
		tm.m[2][2] = N.vz;
		CQuat tempO;

		MToQ(tm, tempO);

		if (new_focus) {
			mO = tempO;
			new_focus = 0;
		} else {
			Quat_Slerp(mPrevO, tempO, 2047, mO);
		}

		mPos = mTripodPos;
		return;
	}

	CVector old_pos;
	CVector Dist;
	int Spd;
	int Frames;
	int mph;
	VECTOR v1;
	VECTOR v2;
	int speed_lag_dist;
	int temptime;

	old_pos = mPos;
	Dist = mTripod->mVel;

	if (((CBruce *)mTripod)->mPhysicsState == 0 || ((CBruce *)mTripod)->mPhysicsState == 4) {
		{
			int i;

			for (i = 5; i > 0; i--)
				mSpeedBuffer[i] = mSpeedBuffer[i - 1];
		}

		if (Dist.Length() > 100) {
			Dist.vx = 0x64000;
			Dist.vy = 0;
			Dist.vz = 0;
		}

		mSpeedBuffer[0] = M3dMaths_SquareRoot0(Mips_CVecDotProd(&Dist, &Dist)) << 6;
	}

	Spd = 0;

	{
		int i;

		for (i = 0; i < 6; i++)
			Spd += mSpeedBuffer[i];
	}

	Spd /= 6;
	Frames = 2;
	mph = Spd / 34 * 60 / Frames * 3600 / 5278;
	mph >>= 12;
	mSpeed = mph;
	speed_lag_dist = (Camera_CamDist + mph * 4 + prev_speed_lag_dist) >> 1;
	prev_speed_lag_dist = speed_lag_dist;
	v1.vx = 0;
	v1.vy = 0;
	v1.vz = -(speed_lag_dist << 12);

	if (turn_delay_timer >= 4) {
		CQuat wouldbeO = mDesiredO * Quat_FromXRot(-450);
		MATRIX m;
		VECTOR v;
		VECTOR vCheck;
		SLineInfo line_info;

		v.vx = 0;
		v.vy = 0;
		v.vz = -(Camera_CamDist << 12);
		QToM(wouldbeO, m);
		ApplyMatrixLV(&m, &v, &vCheck);

		line_info.StartCoords.vx = mTripodPos.vx;
		line_info.StartCoords.vy = mTripodPos.vy;
		line_info.StartCoords.vz = mTripodPos.vz;
		line_info.EndCoords.vx = mTripodPos.vx + vCheck.vx;
		line_info.EndCoords.vy = mTripodPos.vy + vCheck.vy;
		line_info.EndCoords.vz = mTripodPos.vz + vCheck.vz;
		M3dColij_InitLineInfo(&line_info);

		line_info.RecordTriggerZoneHits = 0;
		CameraCollisionCheck = 1;
		M3dZone_LineToItemNotSCached(&line_info, 1);

		CameraCollisionCheck = 0;

		if (!line_info.pItem)
			mDesiredO = wouldbeO;
	}

	if ((big_air && ((CBruce *)mTripod)->mDoingATrick) || big_air_timer > 0) {
		ReactToCollision();

		if (!big_air_timer)
			big_air_mph = mph;

		if (big_air_mph > 30)
			big_air_mph = 30;

		if (!big_air) {
			temptime = big_air_timer;

			if (temptime <= 0) {
				temptime = 0;
			} else {
				if (!big_air_end_timer) {
					if (temptime >= 8)
						temptime = 7;

					big_air_end_timer = temptime + 1;
				} else {
					temptime = big_air_end_timer - 1;
				}
			}

			if (!temptime) {
				big_air_end_timer = 1;
				big_air_timer = -1;
				new_hang_time = 0;
			}

			temptime = temptime * temptime;
			big_air_end_timer--;
			Camera_CamOffset.vy += temptime * 8000 * big_air_mph / 30;
			v1.vz += big_air_mph * 600 * temptime;
		} else {
			big_air_end_timer = 0;

			if (big_air_timer >= 7) {
				temptime = 49;
				Camera_CamOffset.vy += temptime * 8000 * big_air_mph / 30;
				v1.vz += big_air_mph * 600 * temptime;
				new_hang_time++;
			} else if (big_air_timer >= 0) {
				temptime = big_air_timer * big_air_timer;
				new_hang_time = 0;
				Camera_CamOffset.vy += temptime * 8000 * big_air_mph / 30;
				v1.vz += big_air_mph * 600 * temptime;
			}
		}

		Quat_Slerp(mPrevO, mDesiredO, 222, mO);

		big_air_timer++;
	} else {
		int loosen_up = 0;

		if (loose_cam_timer > 0) {
			loosen_up = 1;
			loose_cam_timer--;
		}

		mDesiredO = mDesiredO * Quat_FromXRot(Camera_CamPitch);
		ReactToCollision();

		if (((CBruce *)mTripod)->m740) {
			if (((CBruce *)mTripod)->mPhysicsState == 3) {
				loosen_up = 1;
				loose_cam_timer = 20;
			} else if (((CBruce *)mTripod)->mPhysicsState == 1) {
				mDesiredO = mDesiredO * Quat_FromXRot(200);
			}
		}

		if (in_pool_timer >= 2 && in_pool_timer < 20)
			Quat_Slerp(mPrevO, mDesiredO, 311, mO);
		else if (loosen_up)
			Quat_Slerp(mPrevO, mDesiredO, (20 - loose_cam_timer) * 13 + 51, mO);
		else
			Quat_Slerp(mPrevO, mDesiredO, normal_slerp, mO);
	}

	if (mTimer < 12)
		mPrevO = mO = mDesiredO;

	MATRIX m;

	QToM(mO, m);
	ApplyMatrixLV(&m, &v1, &v2);

	v1.vx = Camera_CamOffset.vx;
	v1.vy = Camera_CamOffset.vy;
	v1.vz = Camera_CamOffset.vz;
	ApplyMatrixLV(&m, &v1, &real_cam_offset);

	mPos.vx = mTripodPos.vx + v2.vx;
	mPos.vy = mTripodPos.vy + v2.vy;
	mPos.vz = mTripodPos.vz + v2.vz;
	mVel = mPos - old_pos;
}

/* SetCamAngle__7CCamerasUs (0x8007b094) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamAngle(short angle, ushort speed) {}

/* SetCamXZDistance__7CCameraUsUs (0x8007b09c) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamXZDistance(ushort dist, ushort speed) {}

/* SetCamYDistance__7CCamerasUs (0x8007b0a4) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamYDistance(short dist, ushort speed) {}

/* SetCamXOffset__7CCamerasUs (0x8007b0ac) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamXOffset(short offset, ushort speed) {}

/* SetCamYOffset__7CCamerasUs (0x8007b0b4) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamYOffset(short offset, ushort speed) {}

/* SetCamZOffset__7CCamerasUs (0x8007b0bc) */
/* Stubbed out in THPS1: does nothing. */
void CCamera::SetCamZOffset(short offset, ushort speed) {}

/* SetZoom__7CCameraUsUs (0x8007b0c4) */
/* Zooms to `zoom` over `time` frames (a per-frame increment), or at once if time is 0. */
void CCamera::SetZoom(ushort zoom, ushort time) {
	mZoomTime = time;

	if (time)
		mZoomIncrement = (zoom - mZoom) / mZoomTime;
	else
		mZoom = zoom;
}

/* FUN_8007b0fc (0x8007b0fc) */
extern "C" int FUN_8007b0fc(CCamera *p) {
	return p->mZoom;
}

/* the assignment as compiled here: x/y/z/w only, the destination keeps its vptr */
static inline void SetQuat(CQuat &d, const CQuat &q) {
	d.x = q.x;
	d.y = q.y;
	d.z = q.z;
	d.w = q.w;
}

/* FUN_8007b108 (0x8007b108) */
extern "C" void FUN_8007b108(CCamera *a) {
	CamPos *s1;
	CamPos *s2;
	CamPos *s4;

	CamPos::DisplayLabels(100, 50, 200, 200, 200);
	Font_String(200, 40, "Pos A", 200, 200, 200, 0);

	s4 = &a->mPosA;
	s4->Display(200, 50, 80, 0, 0);
	Font_String(300, 40, "Pos B", 200, 200, 200, 0);

	s1 = &a->mPosB;
	s1->Display(300, 50, 80, 0, 0);

	if (Pad[1].Right.Triggered) {
		switch (DAT_800d206a) {
			case 0:
				DAT_800d206a = 1;
				break;
			case 1:
				DAT_800d206a = 1;
				break;
		}

		Pad[1].Right.Triggered = 0;
	}

	if (Pad[1].Left.Triggered) {
		switch (DAT_800d206a) {
			case 0:
				DAT_800d206a = 0;
				break;
			case 1:
				DAT_800d206a = 0;
				break;
		}

		Pad[1].Left.Triggered = 0;
	}

	if (Pad[1].Down.Triggered) {
		switch (DAT_800d2068) {
			case 0:
				DAT_800d2068 = 1;
				break;
			case 1:
				DAT_800d2068 = 2;
				break;
			case 2:
				DAT_800d2068 = 3;
				break;
			case 3:
				DAT_800d2068 = 5;
				break;
			case 5:
				DAT_800d2068 = 6;
				break;
			case 6:
				DAT_800d2068 = 8;
				break;
			case 8:
				DAT_800d2068 = 9;
				break;
			case 9:
				DAT_800d2068 = 10;
				break;
			case 10:
				DAT_800d2068 = 0;
				break;
		}

		Pad[1].Down.Triggered = 0;
	}

	if (Pad[1].Up.Triggered) {
		switch (DAT_800d2068) {
			case 0:
				DAT_800d2068 = 10;
				break;
			case 1:
				DAT_800d2068 = 0;
				break;
			case 2:
				DAT_800d2068 = 1;
				break;
			case 3:
				DAT_800d2068 = 2;
				break;
			case 5:
				DAT_800d2068 = 3;
				break;
			case 6:
				DAT_800d2068 = 5;
				break;
			case 8:
				DAT_800d2068 = 6;
				break;
			case 9:
				DAT_800d2068 = 8;
				break;
			case 10:
				DAT_800d2068 = 9;
				break;
		}

		Pad[1].Up.Triggered = 0;
	}

	switch (DAT_800d206a) {
		case 0:
			s2 = s4;
			break;
		case 1:
			s2 = s1;
			break;
	}

	if (Pad[1].Grind.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromXRot(10));
				break;
			case 6:
				s2->mStickLength -= 10;
				break;
		}
	}

	if (Pad[1].Ollie.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromXRot(-10));
				break;
			case 6:
				s2->mStickLength += 10;
				break;
		}
	}

	if (Pad[1].Flip.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromYRot(-10));
		}
	}

	if (Pad[1].Grab.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromYRot(10));
		}
	}

	if (Pad[1].SpinRight.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromZRot(10));
		}
	}

	if (Pad[1].TapSpinRight.Pressed) {
		switch (DAT_800d2068) {
			case 5:
				SetQuat(s2->mStickO, s2->mStickO * QFromZRot(-10));
		}
	}
}

/* AI__7CCamera (0x8007b7c4) */
/* Demo twin (opcode-identical but for 3 spots); layout by THPS1 offsets
   (CCamera members = demo + 0x2c). Pad is the SControl[2] array at Pad. */
void CCamera::AI() {
	{
		int calc = mCalcTripod;

		Camera_CurrentMikeCamera = mMikeIndex;

		if (calc) {
			mOldTripodPos = mTripodPos;

			if (new_focus)
				mTripodPos = mTripod->mPos;
			else
				mTripodPos = ((mTripod->mPos << 1) + mOldTripodPos) / 3;
		}
	}

	if (mCalcFocus) {
		mOldFocusPos = mFocusPos;

		if (new_focus)
			mFocusPos = mFocus->mPos;
		else
			mFocusPos = ((mFocus->mPos << 1) + mOldFocusPos) / 3;
	}

	if (Pad[0].Camera.Pressed) {
		if (DAT_800d2090)
			DAT_800d208c = 1;

		DAT_800d2090 = 0;
	} else {
		DAT_800d2090 = 1;
	}

	if (Cheat_ScreenShots && DAT_800d208c && Pad[0].Camera.Pressed) {
		Pad[1].SpinRight.Triggered = 0;
		Pad[1].TapSpinRight.Triggered = 0;
		DAT_800d2084 = 0;
		Pad[0].TapSpinLeft.Triggered = 0;
		DAT_800d208c = 0;
		Pad[0].Camera.Triggered = 0;
		Pad[0].Pause.Triggered = 0;
		Mick_CameraTweak ^= 1;
		GameFrozen = Mick_CameraTweak;

		if (Mick_CameraTweak) {
			int y = camYOffset;
			int d = camDist;
			int p = camPitch;
			int z = mZoom;

			old_camYOffset = y;
			old_camDist = d;
			old_camPitch = p;
			DAT_800d2088 = z;
		}
	}

	if (Mick_CameraTweak) {
		if (Pad[1].Pause.Pressed) {
			camYOffset = old_camYOffset;
			camDist = old_camDist;
			camPitch = old_camPitch;
			mZoom = DAT_800d2088;
		}

		if (Pad[0].Flip.Pressed)
			mZoom--;

		if (Pad[0].Grab.Pressed)
			mZoom++;

		if (Pad[0].Grind.Pressed)
			mZoom = 256;

		if (Pad[1].Flip.Pressed)
			extra_YRot -= 10;

		if (Pad[1].Grab.Pressed)
			extra_YRot += 10;

		if (Pad[1].Left.Pressed) {
			if (Pad[1].Camera.Pressed)
				camDist -= 32;
			else
				camDist -= 8;
		}

		if (Pad[1].Right.Pressed) {
			if (Pad[1].Camera.Pressed)
				camDist += 32;
			else
				camDist += 8;
		}

		if (Pad[1].Up.Pressed) {
			if (Pad[1].Camera.Pressed)
				camPitch -= 32;
			else
				camPitch -= 8;
		}

		if (Pad[1].Down.Pressed) {
			if (Pad[1].Camera.Pressed)
				camPitch += 32;
			else
				camPitch += 8;
		}

		camPitch &= 4095;

		if (Pad[1].SpinLeft.Pressed) {
			if (Pad[1].Camera.Pressed)
				camYOffset -= 32;
			else
				camYOffset -= 8;
		}

		if (Pad[1].TapSpinLeft.Pressed) {
			if (Pad[1].Camera.Pressed)
				camYOffset += 32;
			else
				camYOffset += 8;
		}
	}

	{
		int dist = camDist;
		int pitch = camPitch;
		int yoff = camYOffset;

		Camera_CamOffset.vx = 0;
		Camera_CamOffset.vz = 0;
		Camera_CamDist = dist;
		Camera_CamPitch = pitch;
		Camera_CamOffset.vy = yoff << 12;
	}

	if (GamePaused) {
		if (Mick_CameraTweak) {
			CM_Normal();
			MoveToDesiredPos(311);
		}
	} else {
		mTimer++;

		if (mZoomTime) {
			mZoomTime--;
			mZoom += mZoomIncrement;
		}

		pCurrentViewport->Zoom = mZoom;
		Utils_CalcAim(mAngles, mPos, mFocusPos);

		if (((CBruce *)mTripod)->m8FE && mTripodPosYDamper) {
			if (mTripod->mVel.vy < 0)
				jump_uptimer++;
			else
				jump_uptimer--;

			jump_acc += (-mTripod->mVel.vy >> 12) * mTripodPosYDamper;

			if (jump_uptimer > 0)
				mTripodPos.vy += jump_acc;
		} else {
			jump_acc = 0;
			jump_uptimer = 0;
		}

		short damp = mPitchDamper;

		if (damp) {
			int vy = mTripod->mVel.vy;

			if (vy < 0)
				Camera_CamPitch -= damp;

			if (vy > 0)
				Camera_CamPitch -= damp >> 1;

			Camera_CamPitch &= 4095;
		}

		switch (mCameraMode) {
			case 1:
				CM_Normal();
				break;
			case 2:
				CM_NoBigAir();
				break;
			case 23:
				CM_CompetitionIntro();
				goto Done;
		}

		if (((CBruce *)mTripod)->mInAir)
			MoveToDesiredPos(311);
		else if (mTripodPos == mOldTripodPos)
			MoveToDesiredPos(151);
		else if (mSpeed < 4)
			MoveToDesiredPos(151);
		else if (((CBruce *)mTripod)->mPhysicsState == 1) {
			if (mTimer < 18)
				MoveToDesiredPos(311);
			else
				MoveToDesiredPos(151);
		} else
			MoveToDesiredPos(311);

		Utils_CalcAim(mAngles, mPos, mFocusPos);

		if (mShakeAmplitude.vx || *(int *)&mShakeAmplitude.vy) {
			mO = Quat_FromYRot(mShakeAmplitude.vy * SIN(mShakeSpeed.vy * TTime) / 4096) * mO *
			     Quat_FromXRot(mShakeAmplitude.vx * SIN(mShakeSpeed.vx * TTime) / 4096) *
			     Quat_FromZRot(mShakeAmplitude.vz * SIN(mShakeSpeed.vz * TTime) / 4096);

			{
				CSVector oldamp = mShakeAmplitude;

				if (mShakeAmplitude.vx < 0)
					mShakeAmplitude.vx += mShakeDecay.vx;
				else
					mShakeAmplitude.vx -= mShakeDecay.vx;

				if (mShakeAmplitude.vy < 0)
					mShakeAmplitude.vy += mShakeDecay.vy;
				else
					mShakeAmplitude.vy -= mShakeDecay.vy;

				if (mShakeAmplitude.vz < 0)
					mShakeAmplitude.vz += mShakeDecay.vz;
				else
					mShakeAmplitude.vz -= mShakeDecay.vz;

				if (((ushort)oldamp.vx ^ (ushort)mShakeAmplitude.vx) & 0x8000)
					mShakeAmplitude.vx = 0;

				if (((ushort)oldamp.vy ^ (ushort)mShakeAmplitude.vy) & 0x8000)
					mShakeAmplitude.vy = 0;

				if (((ushort)oldamp.vz ^ (ushort)mShakeAmplitude.vz) & 0x8000)
					mShakeAmplitude.vz = 0;
			}
		}
	Done:
		LoadIntoMikeCamera();
	}
}

/* CM_Normal__7CCamera (0x8007c240) */
/* Fuzzy demo twin (opcode-identical but for one instruction); locals from its
   MAIN.SYM. THPS1's CCamera members sit 0x2c past the demo's (its CBody is bigger). */
void CCamera::CM_Normal() {
	CVector vUp;
	CVector vRight;
	CSVector ang1;
	int dot_up;
	int dot_front;
	CVector temp_vel = mTripodPos - mOldTripodPos;

	vUp = ((CBruce *)mTripod)->mUp;
	CVector Down(0, 4096, 0);
	CVector Zero(0, 0, 0);
	CVector temp_front;

	dot_up = ABS(Mips_CVecDotProd(&Down, &vUp));
	Utils_CalcAim(ang1, Zero, temp_vel);
	Utils_GetVecFromMagDir(temp_front, 4096, ang1);

	dot_front = ABS(Mips_CVecDotProd(&temp_front, &vUp));

	in_pool_timer++;

	if (dot_up < 3572 &&
	    (((CBruce *)mTripod)->mPhysicsState == 2 || ((CBruce *)mTripod)->mPhysicsState == 8)) {
		if (!big_air && ((CBruce *)mTripod)->m740)
			((CBruce *)mTripod)->mBigAirs++;

		if (++turn_delay_timer >= 4)
			vFront = CVector(0, 0x1000000, 0);
		else
			Utils_GetVecFromMagDir(vFront, 4096, ang1);

		big_air = 1;
		in_pool_timer = 0;
	} else if ((dot_front >> 12) > 2000 || temp_vel.Length() < 5) {
		turn_delay_timer = 0;

		if (dot_up < 3572 && big_air == 1)
			in_pool_timer = 0;

		big_air = 0;
	} else {
		turn_delay_timer = 0;
		Utils_GetVecFromMagDir(vFront, 4096, ang1);

		if (dot_up < 3572 && big_air == 1)
			in_pool_timer = 0;

		big_air = 0;
	}

	mOldVFront = mVFront;

	if (mTripodPos == mOldTripodPos)
		mVFront = mOldVFront;
	else
		mVFront = (mOldVFront + vFront) >> 1;

	if (mTimer < 12) {
		mVFront = (-GSkater[0]->mFront) << 12;
		mOldVFront = mVFront;
		vFront = mOldVFront;
	}

	vFront = mVFront;

	CVector U;
	CVector V;
	CVector N;

	N = vFront >> 12;
	V.vx = -vUp.vx;
	V.vy = -vUp.vy;
	V.vz = -vUp.vz;
	geom_cross(&V, &N, &U, 0);
	U.vx >>= 12;
	U.vy >>= 12;
	U.vz >>= 12;
	geom_cross(&U, &V, &N, 0);
	N.vx >>= 12;
	N.vy >>= 12;
	N.vz >>= 12;
	tm.m[0][0] = U.vx;
	tm.m[1][0] = U.vy;
	tm.m[2][0] = U.vz;
	tm.m[0][1] = V.vx;
	tm.m[1][1] = V.vy;
	tm.m[2][1] = V.vz;
	tm.m[0][2] = N.vx;
	tm.m[1][2] = N.vy;
	tm.m[2][2] = N.vz;
	MToQ(tm, mDesiredO);

	mDesiredO = mDesiredO * Quat_FromYRot(extra_YRot);
}

/* StartCompetitionIntro__7CCamera (0x8007c814) */
/* Switches to the competition-intro camera mode and resets the marquee. */
void CCamera::StartCompetitionIntro() {
	mCompIntroTimer = 0;
	mCameraMode = CAMERAMODE_COMPETITIONINTRO;
	Competition_ResetMarquee();
}

/* CM_CompetitionIntro__7CCamera (0x8007c83c) */
/* Fuzzy demo twin (0.92 by opcode); locals from its MAIN.SYM. The competition
   intro: on the first frame, settle the world and place the camera behind the skater; then slerp
   from the level's intro orientation to the desired one over 130 ticks while sliding the camera in
   from the intro position (X on either pad speeds it up), and start the run when done. The later
   locals are declared mid-block so their slots reuse the freed CQuat temporaries, as retail's do. */
void CCamera::CM_CompetitionIntro() {
	if (!mCompIntroTimer) {
		VECTOR v1;
		VECTOR v2;
		int speed_lag_dist;
		MATRIX m;

		mCameraMode = CAMERAMODE_NORMAL;
		GSkater[0]->mVel.vx = GSkater[0]->mFront.vx * 20;
		GSkater[0]->mVel.vy = GSkater[0]->mFront.vy * 20;
		GSkater[0]->mVel.vz = GSkater[0]->mFront.vz * 20;

		{
			int i;

			for (i = 0; i < 10; i++)
				Ob_AI(&CameraList);
		}

		GSkater[0]->mVel.vx = 0;
		GSkater[0]->mVel.vy = 0;
		GSkater[0]->mVel.vz = 0;
		mCameraMode = CAMERAMODE_COMPETITIONINTRO;
		speed_lag_dist = Camera_CamDist >> 1;
		prev_speed_lag_dist = speed_lag_dist;
		v1.vx = 0;
		v1.vy = 0;
		v1.vz = -(speed_lag_dist << 12);
		QToM(mO, m);
		ApplyMatrixLV(&m, &v1, &v2);

		v1.vx = Camera_CamOffset.vx;
		v1.vy = Camera_CamOffset.vy;
		v1.vz = Camera_CamOffset.vz;
		ApplyMatrixLV(&m, &v1, &real_cam_offset);

		mPos.vx = mTripodPos.vx + v2.vx;
		mPos.vy = mTripodPos.vy + v2.vy;
		mPos.vz = mTripodPos.vz + v2.vz;
		mCompIntroEndPos.vx = mPos.vx;
		mCompIntroEndPos.vy = mPos.vy;
		mCompIntroEndPos.vz = mPos.vz;
	}

	CQuat begin_o;
	CQuat end_o;

	end_o = mDesiredO;
	begin_o = Quat_FromYRot(Levels[GLevel].IntroStartAngY) *
	          Quat_FromXRot(Levels[GLevel].IntroStartAngX) *
	          Quat_FromZRot(Levels[GLevel].IntroStartAngZ);

	{
		int t = (uint)(mCompIntroTimer << 12) / 130;

		Quat_Slerp(begin_o, end_o, t, mO);
	}

	if ((uint)mCompIntroTimer > 130) {
		Video_Start(MechList, VIDEO_RECORD, 1, 0x7fffffff);

		mCameraMode = CAMERAMODE_NORMAL;
		return;
	}

	Competition_MarqueeDisplay();
	CVector begin_pos;
	CVector offset_inc;

	begin_pos.vx = Levels[GLevel].IntroStartPosX << 12;
	begin_pos.vy = Levels[GLevel].IntroStartPosY << 12;
	begin_pos.vz = Levels[GLevel].IntroStartPosZ << 12;
	offset_inc = (begin_pos - mCompIntroEndPos) / 130;
	CVector offset = offset_inc * 130 - mCompIntroTimer * offset_inc;
	int activated;

	mPos.vx = mCompIntroEndPos.vx + offset.vx;
	mPos.vy = mCompIntroEndPos.vy + offset.vy;
	mPos.vz = mCompIntroEndPos.vz + offset.vz;
	activated = Pad[0].MenuAccept.Pressed || Pad[1].MenuAccept.Pressed;

	if (activated)
		mSpeedUpIntro = 1;

	if (mSpeedUpIntro)
		mCompIntroTimer += 6;
	else
		mCompIntroTimer++;
}

/* InCompetitionIntro__7CCamera (0x8007cd64) */
/* True while the camera runs the competition intro. */
int CCamera::InCompetitionIntro() {
	return mCameraMode == CAMERAMODE_COMPETITIONINTRO;
}

/* CM_NoBigAir__7CCamera (0x8007cd78) */
/* Demo twin (0.92 by opcode; THPS1 always uses the tripod's up vector); locals from
   its MAIN.SYM. Camera mode for normal skating: smooths the travel direction into vFront, builds
   a look basis from it and the skater's up vector, and aims mDesiredO along it (plus extra_YRot).
   Down, Zero and U/V/N are declared where retail's frame shows them starting. */
void CCamera::CM_NoBigAir() {
	CVector vUp;
	CVector vRight;
	CSVector ang1;
	int dot_front;
	CVector temp_vel = mTripodPos - mOldTripodPos;

	vUp = ((CBruce *)mTripod)->mUp;
	CVector Down(0, 4096, 0);
	CVector Zero(0, 0, 0);
	CVector temp_front;

	ABS(Mips_CVecDotProd(&Down, &vUp));
	Utils_CalcAim(ang1, Zero, temp_vel);
	Utils_GetVecFromMagDir(temp_front, 4096, ang1);

	dot_front = ABS(Mips_CVecDotProd(&temp_front, &vUp)) >> 12;

	if (dot_front > 2000 || temp_vel.Length() < 5) {
		big_air = 0;
	} else {
		big_air = 0;
		Utils_GetVecFromMagDir(vFront, 4096, ang1);
	}

	mOldVFront = mVFront;

	if (mTripodPos == mOldTripodPos)
		mVFront = mOldVFront;
	else
		mVFront = (mOldVFront + vFront) >> 1;

	vFront = mVFront;
	CVector U;
	CVector V;
	CVector N;

	N = vFront >> 12;
	V.vx = -vUp.vx;
	V.vy = -vUp.vy;
	V.vz = -vUp.vz;
	geom_cross(&V, &N, &U, 0);
	U.vx >>= 12;
	U.vy >>= 12;
	U.vz >>= 12;
	geom_cross(&U, &V, &N, 0);
	N.vx >>= 12;
	N.vy >>= 12;
	N.vz >>= 12;
	tm.m[0][0] = U.vx;
	tm.m[1][0] = U.vy;
	tm.m[2][0] = U.vz;
	tm.m[0][1] = V.vx;
	tm.m[1][1] = V.vy;
	tm.m[2][1] = V.vz;
	tm.m[0][2] = N.vx;
	tm.m[1][2] = N.vy;
	tm.m[2][2] = N.vz;
	MToQ(tm, mDesiredO);

	mDesiredO = mDesiredO * Quat_FromYRot(extra_YRot);
	big_air = 0;
}

/* __6CamPos (0x8007d1a4) */
/* A camera position: its three quaternions start as the identity. */
CamPos::CamPos() {}

/* FUN_8007d20c (0x8007d20c) */
void FUN_8007d20c(char **p, int f) {
	p[0] = __vt_6CamPos;
	p[30] = DAT_800a35c4;
	p[17] = DAT_800a35c4;
	p[7] = DAT_800a35c4;
	p[0] = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* Display__6CamPosUsUsUcUcUc (0x8007d258) */
/* Members from the demo's MAIN.SYM (the calc_/lock_ flags have no type there). */
void CamPos::Display(ushort x, ushort y, uchar r, uchar g, uchar b) {
	if (calc_mTripod) {
		Font_String(x, y, "Follow", r, g, b, 0);
		Font_Number(x, y + 8, mTripod->mPos.vx >> 12, r, g, b, 0);
		Font_Number(x, y + 16, mTripod->mPos.vy >> 12, r, g, b, 0);
		Font_Number(x, y + 24, mTripod->mPos.vz >> 12, r, g, b, 0);
	} else {
		Font_String(x, y, "Fixed", r, g, b, 0);
		Font_Number(x, y + 8, mTripodPos.vx >> 12, r, g, b, 0);
		Font_Number(x, y + 16, mTripodPos.vy >> 12, r, g, b, 0);
		Font_Number(x, y + 24, mTripodPos.vz >> 12, r, g, b, 0);
	}

	Font_String(x, y + 40, "quaternion", r, g, b, 0);
	Font_Number(x, y + 48, mStickLength, r, g, b, 0);
	Font_Number(x, y + 64, mStickOffset.vx >> 12, r, g, b, 0);
	Font_Number(x, y + 72, mStickOffset.vy >> 12, r, g, b, 0);
	Font_Number(x, y + 80, mStickOffset.vz >> 12, r, g, b, 0);
}

/* DisplayLabels__6CamPosUsUsUcUcUc (0x8007d4b4) */
/* Debug overlay: the field labels printed next to CamPos::Display's values (a static
   member: it reads no camera state). */
void CamPos::DisplayLabels(ushort x, ushort y, uchar r, uchar g, uchar b) {
	ushort x2;

	Font_String(x, y, "Tripod", r, g, b, 0);

	x2 = x + 20;
	Font_String(x2, y + 8, "x", r, g, b, 0);
	Font_String(x2, y + 16, "y", r, g, b, 0);
	Font_String(x2, y + 24, "z", r, g, b, 0);
	Font_String(x, y + 40, "Stick O", r, g, b, 0);
	Font_String(x, y + 48, "Stick Length", r, g, b, 0);
	Font_String(x, y + 56, "Stick Offset", r, g, b, 0);
	Font_String(x2, y + 64, "x", r, g, b, 0);
	Font_String(x2, y + 72, "y", r, g, b, 0);
	Font_String(x2, y + 80, "z", r, g, b, 0);
}

/* FUN_8007d6b8 (0x8007d6b8) */
void FUN_8007d6b8(void) {
	volatile int pad[4];

	testquat3.__vptr = DAT_800a35c4;
	testquat2.__vptr = DAT_800a35c4;
	UpDownPosition_8.__vptr = DAT_800a35c4;
}

/* the static initialiser's identity quaternion: vptr first, then x/y/z/w */
static inline void InitStaticQuat(CQuat &q) {
	q.__vptr = DAT_800a35b4;
	q.x = 0;
	q.y = 0;
	q.z = 0;
	q.w = 0x1000;
}

/* FUN_8007d6e4 (0x8007d6e4) */
void FUN_8007d6e4(void) {
	DAT_800d3ab4.Set(0, (int)0xffe70000, (int)0x0);
	FlyFocus.Set(0, (int)0xfff9c000, (int)0x0);
	DAT_800d3a94.Set(0, (int)0xfff60000, (int)0xffc4a000);
	DAT_800d3a84.Set(0, (int)0xfff9c000, (int)0x0);
	DAT_800d3a74.Set(0, (int)0xfff6a000, (int)0x28a000);
	DAT_800d3a64.Set(0, (int)0xfff9c000, (int)0x0);
	DAT_800d3a54.Set(0, (int)0xffbb4000, (int)0x514000);
	DAT_800d3a44.Set(0, (int)0xfff9c000, (int)0xffce0000);
	DAT_800d3a34.Set(0, (int)0xff736000, (int)0x190000);
	DAT_800d3a24.Set(0, (int)0xfff9c000, (int)0xffed4000);
	DAT_800d3a14.Set(0, (int)0x64000, (int)0xf92a0000);
	DAT_800d3a04.Set(0, (int)0xfff9c000, (int)0x0);
	BigShakeAmp.Set(25, 0, 50);
	MediumShakeAmp.Set(0, 0, 30);
	SmallShakeAmp.Set(0, 0, 25);
	LandShakeDecay = CFriction(3, 0, 3);
	LandShakeSpeed.Set(600, 0, 600);
	DemoFocus_8.Set(0, (int)0xfff92000, 0);
	InitStaticQuat(UpDownPosition_8);
	InitStaticQuat(testquat2);
	InitStaticQuat(testquat3);
}

/* FUN_8007d900 (0x8007d900) */
void FUN_8007d900(char **p, int f) {
	p[0] = DAT_800a3594;
	p[2] = DAT_800a35c4;
	p[0] = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_8007d944 (0x8007d944) */
void FUN_8007d944(char *p, int f) {
	*(char **)(p + 0) = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_8007d978 (0x8007d978) */
void FUN_8007d978(char *p, int f) {
	*(char **)(p + 0) = DAT_800a35c4;

	if (f & 1)
		__dl__6CClassPv(p);
}
