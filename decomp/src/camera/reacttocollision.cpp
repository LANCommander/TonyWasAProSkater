/* ReactToCollision__7CCamera (0x80078f3c) */
/* C++ (CAMERA.CPP). Demo twin (opcode-identical but for a few `lui`s); locals and frame from its
   MAIN.SYM. CCamera members are the demo's + 0x2c. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/camera.h"
#include "game/bruce.h"
#include "game/colij.h"
#include "game/m3d.h"
#include "game/vector.h"

extern int GamePaused;
extern int GameFrozen;
extern int CameraCollisionCheck;
extern int GState;

int Camera_CamDist;

void QToM(const CQuat &, MATRIX &);
void M3dColij_InitLineInfo(SLineInfo *);
void Utils_CalcUnit(CVector &, const CVector &);

CQuat Quat_FromXRot(short);
CQuat Quat_FromYRot(short);
CQuat QFromV(const CVector &);
void Quat_Slerp(CQuat, CQuat, int, CQuat &);

static inline int AvgS(short a, short b) {
	return (a + b) >> 1;
}

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

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
		gte_ldopv1(&vVel);
		gte_ldopv2(&vStick);
		gte_op0();
		gte_stlvnl(&vColCheck);

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
