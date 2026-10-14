/* __9CSmokeJetRC7CVectorRC8CSVectoriUsiiiiUcUi (0x80015820) */
/* C++ (SMOKE.CPP). Classes and locals from the demo's MAIN.SYM; CBit's vptr sits at 0x44, after its data. */
#include "game/bit.h"

int Rnd(int);

CSmokeJet::CSmokeJet(const CVector &BasePos, const CSVector &Angles, int Length, ushort Duration,
                     int Damage, int r, int g, int b, uchar ScrollSpeed, uint Checksum)
    : CTexturedRibbon(10, 0) {
	mAngles = Angles;
	mLength = Length;
	SetPos(BasePos);

	mLifetime = Duration;
	mDamage = Damage;

	if (mDamage >= 1000) {
		mDamageBaddies = 1;
		mDamage = Damage - 1000;
	}

	mScrollSpeed = ScrollSpeed;
	SetTexture(Checksum);

	for (int i = 0; i < 10; i++) {
		int scaledr;
		int scaledg;
		int scaledb;

		mOrgX[i] = mpPoints[i].Pos.vx;
		mPhase[i] = Rnd(4096);
		mFreq[i] = 80;
		mAmp[i] = i ? Length / 80 : 0;
		mWidthWobble[i].TChange = Rnd(16);

		if (i == 0)
			mWidthWobble[0].TargetWidth = Rnd(60) + 20;
		else
			mWidthWobble[i].TargetWidth = mWidthWobble[i - 1].TargetWidth * 148 / 128;

		mpPoints[i].Width = mWidthWobble[i].TargetWidth;
		mpPoints[i].WidthB = mpPoints[i].Width + 40;
		scaledr = (9 - i) * r / 9;
		scaledg = (9 - i) * g / 9;
		scaledb = (9 - i) * b / 9;
		mpPoints[i].r = scaledr;
		mpPoints[i].g = scaledg;
		mpPoints[i].b = scaledb;
		scaledr = scaledr * 48 / 256;
		scaledg = scaledg * 48 / 256;
		scaledb = scaledb * 48 / 256;
		mpPoints[i].rB = scaledr;
		mpPoints[i].gB = scaledg;
		mpPoints[i].bB = scaledb;
	}
}
