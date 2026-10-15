/* AI__7CCamera (0x8007b7c4) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Demo twin (opcode-identical but for 3 spots); layout by THPS1 offsets
   (CCamera members = demo + 0x2c). Pad is the SControl[2] array at Pad. */
#include "game/camera.h"
#include "game/bruce.h"
#include "game/pad.h"
#include "game/m3d.h"

struct SSinCos {
	short s, c;
};

extern SSinCos rcossin_tbl;

#define SIN(a) ((&rcossin_tbl)[(a) & 4095].s)

extern SControl Pad[2];
extern CVector Camera_CamOffset;
extern int Cheat_ScreenShots;
extern int GameFrozen;
extern int GamePaused;
extern SViewport *pCurrentViewport;
extern int TTime;

extern int Camera_CurrentMikeCamera;
extern int camDist;
extern int camPitch;
extern int camYOffset;
extern int Camera_CamPitch;
extern int Camera_CamDist;
extern int Mick_CameraTweak;
extern int DAT_800d2084;
extern int DAT_800d2088;
extern int DAT_800d208c;
extern int DAT_800d2090;
extern int old_camYOffset;
extern int old_camDist;
extern int old_camPitch;

CQuat Quat_FromXRot(short);
CQuat Quat_FromYRot(short);
CQuat Quat_FromZRot(short);
int Utils_CalcAim(CSVector &, const CVector &, const CVector &);

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
