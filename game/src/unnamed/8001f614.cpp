/* __6CLightRC7CVectoriiiiiUcUcUcUcUcUc (0x8001f614) */
/* C++ (LIGHT.CPP). A dynamic light: a non-rendered bit at Pos with an inner/fringe radius, two colours
   and an amplitude; it starts switched on. */
#include "game/bit.h"

CLight::CLight(const CVector &Pos, int Node, int ClipDist, int InnerRadius, int FringeRadius,
               int Amp, uchar r0, uchar g0, uchar b0, uchar r1, uchar g1, uchar b1) {
	mPos = Pos;
	mClipDist = ClipDist;
	mType = 2;
	mLifetime = Node; /* a light keeps its trigger node in mLifetime */
	mOn = 1;
	mInnerRadius = InnerRadius;
	mFringeRadius = FringeRadius;
	mr0 = r0;
	mg0 = g0;
	mb0 = b0;
	mr1 = r1;
	mg1 = g1;
	mb1 = b1;
	mAmp = Amp;
}
