/* AlignToFallLine__6CBrucei (0x8003a214) */
/* C++ (PHYSICS.CPP). Fuzzy demo twin (0.72; retail is a trimmed version), locals from its
   MAIN.SYM. When the skater is moving across a slope, turns them towards the fall line: the turn
   rate grows with the slope's steepness and shrinks with speed. It's held at 0 while the skater
   faces within a dead zone either side of the fall line's perpendicular. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"
#include "game/vector.h"

extern int XFrames;

void M3dUtils_NormaliseVector(CVector *);
void ProjectCVectorToPlane(CVector *, SVECTOR);

void CBruce::AlignToFallLine(int Amount) {
	int Speed;
	int dot;
	int PolyTilt;
	int angle;
	CVector FallLinePerp;
	CVector FallLineUnit;
	VECTOR mLPN;
	CVector FrontInPlane;

	mFallLineTurn = 0;

	if (mInAir)
		return;

	Speed = SquareRoot0(Mips_CVecDotProd(&mVel, &mVel)) << 6;
	PolyTilt = SquareRoot0(Mips_CVecDotProd(&mFallLine, &mFallLine)) << 6;
	angle = PolyTilt / 260;
	angle = angle * PolyTilt / 13000;

	if (mMaxSpeed < Speed)
		Speed = mMaxSpeed;

	angle = angle * ((Amount << 12) + mMaxSpeed - Speed) / mMaxSpeed;

	if (!angle)
		return;

	if (mFallLineFrame + 10 < XFrames)
		mFallLineSide = 0;

	mFallLineFrame = XFrames;
	FallLineUnit = mFallLine;
	M3dUtils_NormaliseVector(&FallLineUnit);

	mLPN.vx = mPolyNormal.vx;
	mLPN.vy = mPolyNormal.vy;
	mLPN.vz = mPolyNormal.vz;
	geom_cross(&mLPN, &FallLineUnit, &FallLinePerp, 12);

	FrontInPlane = mFront;
	ProjectCVectorToPlane(&FrontInPlane, mPolyNormal);

	dot = Mips_CVecDotProd(&FrontInPlane, &FallLineUnit);
	dot = Mips_CVecDotProd(&mFront, &FallLinePerp);

	if (dot >= 42) {
		if (mFallLineSide != 2) {
			mFallLineTurn = -angle;
			mFallLineSide = 1;
		}
	} else if (dot < -41) {
		if (mFallLineSide != 1) {
			mFallLineTurn = angle;
			mFallLineSide = 2;
		}
	} else {
		mFallLineSide = 0;
	}
}
