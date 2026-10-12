/* __8CPowerUpUsRC7CVectorT2Uii (0x8001f934) */
/* C++ (POWERUP.CPP). Fuzzy demo twin; body from the earlier C draft, as a real constructor so `this`
   is copied where retail copies it. Layout by THPS1 offsets; callees keep their linker names. */
#include "game/powerup.h"

extern char s_skmedals[];
extern char s_items_800d161c[];
extern int GGame;
extern CPowerUp *GPowS;
extern CPowerUp *GPowK;
extern CPowerUp *GPowA;
extern CPowerUp *GPowT;
extern CPowerUp *GPowE;
extern CPowerUp *GPowTape;
extern unsigned char CurrentItemsFileRegion;
extern unsigned char CurrentMedalsFileRegion;

char PowerUpList[8];

extern "C" int Redbook_Rnd__Fi(int);
extern "C" short Spool_GetModel__FUii(unsigned, int);
extern "C" int Utils_GetGroundHeight__FRC7CVectorii(CVector *, int, int);

CPowerUp::CPowerUp(ushort a1, const CVector &a2_, const CVector &a3_, uint a4, int a5) {
	const CVector *a2 = &a2_, *a3 = &a3_;

	char *c;

	this->mTimer = 0;
	this->mType = a1;
	this->mHoverHeight = 0;
	this->mLifetime = a5;
	this->mOrgPos = *a2;
	this->mPos = *a2;

	if (a4 & 8) {
		this->mVel.vy = 0xfffc4000;
	} else {
		this->mVel = *a3;
	}

	c = (char *)&this->mFric;
	this->mFric.vx = 5;
	c[1] = 5;
	c[2] = 5;
	this->mAngVel.vy = 100;
	this->mAcc.vy = 0x8000;
	this->mWobbleSpeed = Redbook_Rnd__Fi(20) + 90;
	this->mMaxWobbleAmplitude = 50;
	this->mWobbleAmplitude = 50;

	if (a4 & 16) {
		this->mDropping = 1;
		this->mDropForever = 1;
	} else if (a4 & 4) {
		this->mDropping = 1;
		this->mGroundY = Utils_GetGroundHeight__FRC7CVectorii(&this->mOrgPos, 0, 8000);
	}

	this->mRMinor = 300;
	ShadowOn();

	this->mShadowThreshold = 300;
	InitItem(s_items_800d161c);

	switch (this->mType) {
		case 0x664:
			InitItem(s_skmedals);

			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x54636518, CurrentMedalsFileRegion);
			break;
		case 0x665:
			InitItem(s_skmedals);

			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0xba6d0434, CurrentMedalsFileRegion);
			break;
		case 0x666:
			InitItem(s_skmedals);

			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x2364558e, CurrentMedalsFileRegion);
			break;
		case 5:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x311d55d4, CurrentItemsFileRegion);
			break;
		case 4:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x2328a71c, CurrentItemsFileRegion);
			break;
		case 6:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x2ebf22ca, CurrentItemsFileRegion);
			break;
		case 15:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x34524351, CurrentItemsFileRegion);
			break;
		case 10:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x29b68a16, CurrentItemsFileRegion);
			break;
		case 18:
		case 16:
			this->mAngVel.vy = 150;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0xd5108e8c, CurrentItemsFileRegion);
			break;
		case 21:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x6f0f6b9f, CurrentItemsFileRegion);
			break;
		case 22:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0xf6063a25, CurrentItemsFileRegion);
			break;
		case 23:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel__FUii(0x81010ab3, CurrentItemsFileRegion);
			break;
	}

	if (GGame != 1) {
		switch (this->mType) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
			case 16:
				this->mLifetime = 1;
				break;
		}
	} else {
		switch (this->mType) {
			case 5:
				GPowS = this;
				break;
			case 4:
				GPowK = this;
				break;
			case 6:
				GPowA = this;
				break;
			case 15:
				GPowT = this;
				break;
			case 10:
				GPowE = this;
				break;
			case 16:
				GPowTape = this;
				break;
		}
	}

	AttachTo((CBody **)PowerUpList);

	if (this->mIs3d == 0)
		DontDisplay();
}
