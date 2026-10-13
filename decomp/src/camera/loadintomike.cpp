/* LoadIntoMikeCamera__7CCamera (0x80078b04) flags=-fstructure-noalias */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/camera.h"
#include "game/bruce.h"

extern SCamera MikeCamera[];
extern CVector Camera_CamOffset;

int Camera_CurrentMikeCamera;

void QToM(const CQuat &, MATRIX &);

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
