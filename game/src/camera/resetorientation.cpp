/* ResetOrientation__7CCamera (0x80078d94) */
#include "game/camera.h"
#include "game/bruce.h"

unsigned int Camera_CurrentMikeCamera;

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
