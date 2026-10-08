/* __6CBruce (0x80041e5c) flags=-fstructure-noalias */
/* C++ (MECH.CPP). Demo twin; statement order from its line table. The skater constructor: an
   inline CPanelData ctor (retail has it inline, demo out of line), then the per-level/game-mode
   defaults, the two kick speeds from stat 1, the identity-ish rotation, list attach and
   shadow setup. */
#pragma interface

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

struct CVector {
	int vx, vy, vz;
	void Set(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

struct CSVector {
	short vx, vy, vz;
	void Set(short x, short y, short z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

struct SSkater {
	char pad0[0x18];
	int Stats[10]; /* 0x18 */
	char pad40[340 - 0x40];
};

struct SLevel {
	char pad0[0xc];
	int mC; /* 0x0c */
	char pad10[0x78 - 0x10];
};

struct SFront {
	char pad0[0x584];
	int m584;
};

extern SSkater DAT_800a44fc[];
extern SLevel Levels[];
extern int GLevel;
extern int GGame;
extern int Cheat_StatsOverride;
extern int Cheat_Fire;
extern int GNumberOfPlayers;
extern SFront *CameraList;
extern int DAT_800a8118[];
extern int Pad[];
extern int Pad_16c[];

class CBruce;

class TextEffect {
  public:
	char pad0[72];
	TextEffect();
};

class CPanelData {
  public:
	CBruce *mpBruce;  /* 0x000 */
	uint mTotalScore; /* 0x004 */
	char pad8[0x154 - 0x8];
	int mLastHoldingScore; /* 0x154 */
	int mLastMult;         /* 0x158 */
	char pad15c[0x164 - 0x15c];
	TextEffect mEffect1; /* 0x164 */
	int m1AC;            /* 0x1ac */
	TextEffect mEffect2; /* 0x1b0 */
	char pad1f8[0x388 - 0x1f8];
	int mTQStack; /* 0x388 */
	char pad38c[0x4bc - 0x38c];
	int m4BC; /* 0x4bc */
	char pad4c0[0x4c8 - 0x4c0];
	int SKATE[5]; /* 0x4c8 */
	int Goals;    /* 0x4dc */
	int m4E0;     /* 0x4e0 */
	int m4E4;     /* 0x4e4 */
	char pad4e8[0x4f4 - 0x4e8];
	int m4F4; /* 0x4f4 */
	CPanelData(CBruce *pBruce)
	    : mpBruce(pBruce), mTotalScore(0), mLastHoldingScore(0), mLastMult(0), m1AC(0) {
		mTQStack = 0;
		m4BC = 0;
		m4F4 = 0;

		for (int i = 0; i < 5; i++)
			SKATE[i] = 0;

		m4E4 = 0;
		m4E0 = 0;
		Goals = 0;
	}
};

class CBody;

struct CRGB {
	uchar r, g, b;
	void Set(uchar x, uchar y, uchar z) {
		r = x;
		g = y;
		b = z;
	}
};

class CItem {
  public:
	ushort mFlags; /* 0x000 */
	char pad2[0x24 - 0x2];
	CSVector mScale; /* 0x024 */
	char pad2a[0x3c - 0x2a];
	void *mp3C; /* 0x03c */
	char pad40[0x48 - 0x40];
	virtual ~CItem();
};

class CSuper : public CItem {
  public:
	char pad4c[0x52 - 0x4c];
	ushort m52; /* 0x052 */
	char pad54[0x94 - 0x54];
	CRGB m94; /* 0x094 */
	char pad97;
	short m98; /* 0x098 */
	char pad9a[0xa8 - 0x9a];
	CSVector mAngles; /* 0x0a8 */
	char padae[0x134 - 0xae];
	int m134; /* 0x134 */
	char pad138[0x150 - 0x138];
	int m150; /* 0x150 */
	char pad154[0x15a - 0x154];
	uchar m15A; /* 0x15a */
	char pad15b;
	short *mpShadowPos; /* 0x15c */
	uchar *mpShadowCol; /* 0x160 */
	char pad164[0x190 - 0x164];
	CSuper();
	void AttachTo(CBody **);
	void ShadowOn();
	void UpdateShadow();
};

class CBruce : public CSuper {
  public:
	char pad190[0x198 - 0x190];
	uchar *mpStatBonus; /* 0x198 */
	char pad19c[0x1c8 - 0x19c];
	CVector m1C8; /* 0x1c8 */
	char pad1d4[0x254 - 0x1d4];
	int m254; /* 0x254 */
	char pad258[0x2e8 - 0x258];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x330 - 0x2ec];
	int m330;       /* 0x330 */
	int m334;       /* 0x334 */
	int mKickSpeed; /* 0x338 */
	char pad33c[0x340 - 0x33c];
	int m340; /* 0x340 */
	int m344; /* 0x344 */
	char pad348[0x35c - 0x348];
	int m35C; /* 0x35c */
	char pad360[0x424 - 0x360];
	int m424;     /* 0x424 */
	int m428[80]; /* 0x428 */
	int m568[80]; /* 0x568 */
	int m6A8;     /* 0x6a8 */
	char pad6ac[0x6c8 - 0x6ac];
	int m6C8; /* 0x6c8 */
	int m6CC; /* 0x6cc */
	char pad6d0[0x6dc - 0x6d0];
	int m6DC; /* 0x6dc */
	int m6E0; /* 0x6e0 */
	char pad6e4[0x70c - 0x6e4];
	int m70C; /* 0x70c */
	int m710; /* 0x710 */
	char pad714[0x734 - 0x714];
	int m734; /* 0x734 */
	int m738; /* 0x738 */
	int m73C; /* 0x73c */
	int m740; /* 0x740 */
	int m744; /* 0x744 */
	char pad748[0x750 - 0x748];
	int m750; /* 0x750 */
	char pad754[0x764 - 0x754];
	int m764;             /* 0x764 */
	int m768;             /* 0x768 */
	MATRIX mOldRotMatrix; /* 0x76c */
	MATRIX mRotMatrix;    /* 0x78c */
	char pad7ac[0x7fc - 0x7ac];
	CVector m7FC;     /* 0x7fc */
	int m808;         /* 0x808 */
	CSVector mNormal; /* 0x80c */
	char pad812[0x820 - 0x812];
	int m820;     /* 0x820 */
	CVector m824; /* 0x824 */
	char pad830[0x834 - 0x830];
	int m834; /* 0x834 */
	int m838; /* 0x838 */
	int m83C; /* 0x83c */
	int m840; /* 0x840 */
	char pad844[0x86c - 0x844];
	int *mpColl; /* 0x86c */
	char pad870[0x924 - 0x870];
	CPanelData mPanel; /* 0x924 */
	CBruce();
	virtual ~CBruce();
	void FRUVectorsFromRotMatrix();
};

#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride ? Cheat_StatsOverride                                                                   \
	              : (DAT_800a44fc[mSkaterIndex].Stats[n] + ((m254 || Cheat_Fire) ? 3 : 0)) +     \
	                    mpStatBonus[n])

void *Mem_New(uint, int, int);

int NumMechs;
CBody *MechList;

CBruce::CBruce() : mPanel(this) {
	int i;

	m150 = 0x10000;

	if (Levels[GLevel].mC && GGame == 1)
		m330 = 3630;
	else if (GGame != 7)
		m330 = 7230;
	else
		m330 = 630;

	m1C8.Set(0, 1000, 0);

	mFlags |= 0x200;
	mScale.Set(4096, 4096, 4096);

	m334 = ((GetStat(1) + 39) << 12) * 3 * 15 / 28;
	mKickSpeed = ((GetStat(1) + 17) << 12) * 3 * 15 / 28;
	m340 = 30;
	m344 = 30;
	m6DC = 0;
	m6E0 = 0;
	m70C = 0;
	m6C8 = 0;
	m710 = -1;
	m6CC = 0;
	m734 = 1;
	m738 = 1;
	m6A8 = 0;
	m73C = 0;
	m740 = 0;
	m744 = 0;
	m35C = 0;
	m750 = 0;
	m424 = 0;
	m134 |= 1;

	for (i = 0; i < 80; i++)
		m428[i] = 0;

	for (i = 0; i < 80; i++)
		m568[i] = 0;

	m764 = 0;
	m768 = 0;
	m7FC.Set(0, 0, 0);

	m808 = 0;
	mNormal.vx = 0;
	mNormal.vy = -4096;
	mNormal.vz = 0;
	m834 = 0;
	m838 = 0;
	m83C = 0;
	m840 = 0;
	mRotMatrix.m[0][0] = -4096;
	mRotMatrix.m[1][0] = 0;
	mRotMatrix.m[2][0] = 0;
	mRotMatrix.m[0][1] = 0;
	mRotMatrix.m[1][1] = -4096;
	mRotMatrix.m[2][1] = 0;
	mRotMatrix.m[0][2] = 0;
	mRotMatrix.m[1][2] = 0;
	mRotMatrix.m[2][2] = -4096;
	FRUVectorsFromRotMatrix();

	mOldRotMatrix = mRotMatrix;
	int x = mRotMatrix.m[0][1];
	int y = mRotMatrix.m[1][1];
	int z = mRotMatrix.m[2][1];

	m820 = 0;
	m824.vx = x;
	m824.vy = y;
	m824.vz = z;
	m98 = 2;
	mAngles.vx = x;
	mAngles.vy = y;
	mAngles.vz = z;
	AttachTo(&MechList);

	NumMechs++;
	m52 &= ~2;

	if (NumMechs == 1)
		mpColl = Pad;
	else
		mpColl = Pad_16c;

	m94.Set(5, 1, 5);

	if (GNumberOfPlayers == 1) {
		m15A = 1;
		mpShadowPos = (short *)Mem_New(32, 0, 1);
		mpShadowCol = (uchar *)Mem_New(16, 0, 1);
		mpShadowPos[0] = 0;
		mpShadowPos[1] = -4096;
		mpShadowPos[2] = 0;
		mpShadowCol[0] = 128;
		mpShadowCol[1] = 128;
		mpShadowCol[2] = 128;
	} else {
		ShadowOn();
		UpdateShadow();
	}

	CameraList->m584 = 1;
	mp3C = DAT_800a8118;
}
