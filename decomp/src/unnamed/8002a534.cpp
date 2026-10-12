/* __9CGlassBitRC7CVectorT1iUcUcUciii (0x8002a534) */
/* C++ (BIT.CPP). A shard of glass, linked into GlassList: a triangle with each corner placed
   randomly within +-(RangeX, RangeY, RangeZ) units of Pos, a quarter-intensity colour and a random
   30-59 frame hold before it starts to fade. */
#include "game/bit.h"

CGlassBit *GlassList;

int Redbook_Rnd(int);

CGlassBit::CGlassBit(const CVector &Pos, const CVector &Vel, int GroundY, uchar r, uchar g, uchar b, int RangeX,
                     int RangeY, int RangeZ) {
	AttachTo(&GlassList);

	int SpanX = RangeX * 2 + 1;
	int SpanY = RangeY * 2 + 1;
	int SpanZ = RangeZ * 2 + 1;

	mPos = Pos;
	mPosA.vx = Pos.vx + ((Redbook_Rnd(SpanX) - RangeX) << 12);
	mPosA.vy = Pos.vy + ((Redbook_Rnd(SpanY) - RangeY) << 12);
	mPosA.vz = Pos.vz + ((Redbook_Rnd(SpanZ) - RangeZ) << 12);
	mPosB.vx = Pos.vx + ((Redbook_Rnd(SpanX) - RangeX) << 12);
	mPosB.vy = Pos.vy + ((Redbook_Rnd(SpanY) - RangeY) << 12);
	mPosB.vz = Pos.vz + ((Redbook_Rnd(SpanZ) - RangeZ) << 12);
	mPosC.vx = Pos.vx + ((Redbook_Rnd(SpanX) - RangeX) << 12);
	mPosC.vy = Pos.vy + ((Redbook_Rnd(SpanY) - RangeY) << 12);
	mPosC.vz = Pos.vz + ((Redbook_Rnd(SpanZ) - RangeZ) << 12);
	mVel = Vel;
	mGroundY = GroundY;
	mDefaultR = r >> 2;
	mDefaultG = g >> 2;
	mDefaultB = b >> 2;
	mR = r >> 2;
	mG = g >> 2;
	mB = b >> 2;
	mLifetime = Redbook_Rnd(30) + 30;
	mFadeRate = 4;
}
