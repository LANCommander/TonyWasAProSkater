/* __5CTapeUsRC7CVectorT2iiT2Uii (0x80072ee0) */
/* C++ (TAPES.CPP). Locals and statement order from the demo's MAIN.SYM. CItem is the demo's
   layout; THPS1's CBody is 0x2c bytes bigger than the demo's, so its fields are placed by offset. */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

struct CVector {
	int vx, vy, vz;
};

struct CSVector {
	short vx, vy, vz;
	void Set(short x, short y, short z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

struct CFriction {
	uchar x, y, z;
	void Set(uchar a, uchar b, uchar c) {
		x = a;
		y = b;
		z = c;
	}
};

struct SLight;
struct CGlow;
struct STapeMessage;

class CItem {
  public:
	ushort mFlags;
	ushort mInquiry;
	CVector mPos;
	CSVector mAngles;
	ushort mModel;
	char mDummyFrame;
	uchar mTintIndex;
	uchar mDummyAnim;
	uchar mRegion;
	CItem *mNextItem;
	uint mRGB;
	CSVector mScale;
	uint mTRN;
	CItem *mPreviousItem;
	uint mStoredRGB;
	uchar mPredatorEffectType;
	uchar mPredatorEffectFade;
	ushort mType;
	SLight *mpLight;
	ushort mFlashTimer;
	short mFlash_dR;
	short mFlash_dG;
	short mFlash_dB;
	virtual ~CItem();
	void InitItem(char *);
};

class CBody : public CItem {
  public:
	char pad4c[0x6c - 0x4c];
	CVector mVel; /* 0x6c */
	int pad78;
	int m7c; /* 0x7c */
	char pad80[5];
	CFriction mFric; /* 0x85 */
	char pad88[2];
	ushort m8a; /* 0x8a */
	char pad8c[0xe0 - 0x8c];
	ushort me0; /* 0xe0 */
	char pade2[0x134 - 0xe2];
	CBody();
	void AttachTo(CBody **);
};

class CTape : public CBody {
  public:
	CGlow *mpGlow;
	ushort mGlowRadiusOrg;
	ushort mGlowRadiusAmp;
	STapeMessage *mMessageTable;
	int mSessionTapeNum;
	int mTapeNum;
	int mTextX;
	int mTextY;
	int mSize;
	int mSize1;
	int mSize2;
	CVector mVec1;
	CVector mVec2;
	uchar mHasNode;
	uchar mIs3d;
	uchar mDropping;
	uchar mDropForever;
	uchar mHoverHeight;
	ushort mNodeIndex;
	int mGroundY;
	CVector mOrgPos;
	ushort mT;
	ushort mMaxWobbleAmplitude;
	ushort mWobbleAmplitude;
	ushort mWobbleSpeed;
	ushort mLifetime;
	uint mTimer;
	CTape(ushort PowerupType, const CVector &Pos, const CVector &Pos2, int Size1, int Size2,
	      const CVector &Vel, uint Flags, int Lifetime);
	void DontDisplay();
	virtual ~CTape();
};

int Rnd(int);
short Spool_GetModel(uint, int);
int Career_Got(int);

extern char s_items_800d1fb0[];
extern char s_skmedals_800a3050[];
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

	m8a = 100;
	m7c = 0x8000;
	mWobbleSpeed = Rnd(20) + 90;
	mMaxWobbleAmplitude = 50;
	mWobbleAmplitude = 50;
	me0 = 300;
	InitItem(s_items_800d1fb0);

	switch (mType) {
		case 0x664:
			InitItem(s_skmedals_800a3050);

			mIs3d = 1;
			mModel = Spool_GetModel(0x54636518, CurrentMedalsFileRegion);
			break;
		case 0x665:
			InitItem(s_skmedals_800a3050);

			mIs3d = 1;
			mModel = Spool_GetModel(0xba6d0434, CurrentMedalsFileRegion);
			break;
		case 0x666:
			InitItem(s_skmedals_800a3050);

			mIs3d = 1;
			mModel = Spool_GetModel(0x2364558e, CurrentMedalsFileRegion);
			break;
		case 0x10:
		case 0x12:
			mMessageTable = TapeMessage;
			m8a = 150;
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
