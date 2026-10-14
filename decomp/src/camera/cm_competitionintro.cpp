/* CM_CompetitionIntro__7CCamera (0x8007c83c) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Fuzzy demo twin (0.92 by opcode); locals from its MAIN.SYM. The competition
   intro: on the first frame, settle the world and place the camera behind the skater; then slerp
   from the level's intro orientation to the desired one over 130 ticks while sliding the camera in
   from the intro position (X on either pad speeds it up), and start the run when done. The later
   locals are declared mid-block so their slots reuse the freed CQuat temporaries, as retail's do. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
#ifndef PC_PORT
	#include <inline_c.h>
#endif
}

#include "game/camera.h"
#include "game/bruce.h"
#include "game/pad.h"
#include "game/career.h"
#include "game/video.h"

extern SControl Pad[2];
extern SLevel Levels[];
extern CVector Camera_CamOffset;
extern CBruce *GSkater;
extern CBruce *MechList;
extern int GLevel;

int Camera_CamDist;
CBody *CameraList;

void Ob_AI(CBody **);
void QToM(const CQuat &, MATRIX &);
CQuat Quat_FromXRot(short);
CQuat Quat_FromYRot(short);
CQuat Quat_FromZRot(short);
void Quat_Slerp(CQuat, CQuat, int, CQuat &);
void Video_Start(CBruce *, EVideoMode, int, int);
void Competition_MarqueeDisplay();

void CCamera::CM_CompetitionIntro() {
	if (!mCompIntroTimer) {
		VECTOR v1;
		VECTOR v2;
		int speed_lag_dist;
		MATRIX m;

		mCameraMode = CAMERAMODE_NORMAL;
		GSkater->mVel.vx = GSkater->mFront.vx * 20;
		GSkater->mVel.vy = GSkater->mFront.vy * 20;
		GSkater->mVel.vz = GSkater->mFront.vz * 20;

		{
			int i;

			for (i = 0; i < 10; i++)
				Ob_AI(&CameraList);
		}

		GSkater->mVel.vx = 0;
		GSkater->mVel.vy = 0;
		GSkater->mVel.vz = 0;
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
