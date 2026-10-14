/* MoveToDesiredPos__7CCameraUi (0x8007a494) flags=-fstructure-noalias */
/* C++ (CAMERA.CPP). Fuzzy demo twin (0.89 by opcode); locals from its MAIN.SYM. Moves the camera
   each frame. With no tripod it builds a look-at basis from the focus direction and slerps to it.
   Otherwise it averages the tripod's speed into mph and a speed lag, tilts down if a ray behind
   the camera is clear, then either does big-air handling (lifting the offset by hang time) or
   normal tilt/loosen handling, slerps mO toward mDesiredO, and places the camera.
   THPS1 drops the demo's |N.V| test (always the second basis) and the gp camera-offset copy.
   fold() moves a constant onto the *other* factor of a product: `temptime * 8000 * big_air_mph`
   shifts big_air_mph and multiplies by temptime, as retail does. Locals are declared where
   first used so their stack slots reuse the earlier blocks' slots. */
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

extern MATRIX tm;
extern CVector Camera_CamOffset;
extern int CameraCollisionCheck;

int Camera_CamDist;
int Camera_CamPitch;

void QToM(const CQuat &, MATRIX &);
void MToQ(const MATRIX &, CQuat &);
void M3dColij_InitLineInfo(SLineInfo *);
void Utils_CalcUnit(CVector &, const CVector &);
CQuat Quat_FromXRot(short);
void Quat_Slerp(CQuat, CQuat, int, CQuat &);

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
