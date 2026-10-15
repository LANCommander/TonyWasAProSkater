/* __6CSparkR7CVectoriii (0x80029684) flags=-fstructure-noalias */
/* C++ (BIT.CPP). Fuzzy demo twin; members and the inlined CFriction block from its MAIN.SYM, written
   as a real constructor. A spark is a TILE (or a 1x1 TILE_1 when the global spark size is 1) coloured
   from the current spark colour, given a random-ish velocity, gravity and lifetime. Needs
   -fstructure-noalias: retail loads the globals above the vptr and mLifetime stores. */
#include "game/bit.h"

extern uint SparkSize;
extern int SparkSemiTrans;
extern uchar SparkRGB[4], SparkFadeRGB[4]; /* CVECTOR */

short Redbook_Rnd(int);
void Bit_CalculateSparkVelocity(CVector &, int);

CSpark::CSpark(CVector &pos, int vel, int gravity, int life) {
	if ((SparkSize & 15) == 1) {
		code = 0x68;
		tag = 0x02000000;
		mWidthHeight = 1;
	} else {
		code = 0x60;
		mWidthHeight = SparkSize;
		tag = 0x03000000;
	}

	r0 = SparkRGB[0];
	g0 = SparkRGB[1];
	b0 = SparkRGB[2];
	mFadeR = SparkFadeRGB[0];
	mFadeG = SparkFadeRGB[1];
	mFadeB = SparkFadeRGB[2];
	mPos = pos;
	Bit_CalculateSparkVelocity(mVel, vel);

	mAcc.vy = gravity;
	mFric.Set(3, 3, 3);

	mLifetime = Redbook_Rnd(life);

	if (SparkSemiTrans)
		code |= 2;
}
