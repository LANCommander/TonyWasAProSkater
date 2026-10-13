/* __5CTapeUsRC7CVectorT2iiT2Uii (0x80072ee0) */
/* C++ (TAPES.CPP). Locals and statement order from the demo's MAIN.SYM. CItem is the demo's
   layout; THPS1's CBody is 0x2c bytes bigger than the demo's, so its fields are placed by offset. */
#include "game/tapes.h"

int Rnd(int);
short Spool_GetModel(uint, int);
int Career_Got(int);

extern STapeMessage TapeMessage[];
extern CBody *PowerUpList;
extern int GGame;
extern uchar CurrentItemsFileRegion;
extern uchar CurrentMedalsFileRegion;

CTape::CTape(ushort PowerupType, const CVector &Pos, const CVector &Pos2, int Size1, int Size2,
             const CVector &Vel, uint Flags, int Lifetime) {
	mSize = Size1;
	mSize1 = Size1;
	mSize2 = Size2;
	mVec1 = Pos;
	mVec2 = Pos2;
	mScale.Set(Size1, Size1, Size1);

	mTimer = 0;
	mType = PowerupType;
	mLifetime = Lifetime;
	mTapeNum = 0;
	mSessionTapeNum = 0;
	mOrgPos = Pos;
	mPos = Pos;
	mVel = Vel;
	mFric.Set(5, 5, 5);

	mAngVel.vy = 100;
	mAcc.vy = 0x8000;
	mWobbleSpeed = Rnd(20) + 90;
	mMaxWobbleAmplitude = 50;
	mWobbleAmplitude = 50;
	mRMinor = 300;
	InitItem("items");

	switch (mType) {
		case 0x664:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0x54636518, CurrentMedalsFileRegion);
			break;
		case 0x665:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0xba6d0434, CurrentMedalsFileRegion);
			break;
		case 0x666:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0x2364558e, CurrentMedalsFileRegion);
			break;
		case 0x10:
		case 0x12:
			mMessageTable = TapeMessage;
			mAngVel.vy = 150;
			mIs3d = 1;
			mModel = Spool_GetModel(0xd5108e8c, CurrentItemsFileRegion);
			break;
	}

	if (GGame != 1) {
		switch (mType) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
			case 16:
				mLifetime = 1;
				break;
		}
	} else {
		switch (mType) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
				if (Career_Got(2))
					mLifetime = 1;

				break;
			case 16:
				if (Career_Got(3))
					mLifetime = 1;

				break;
		}
	}

	AttachTo(&PowerUpList);

	if (!mIs3d)
		DontDisplay();
}
