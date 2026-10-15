/* CTape (TAPES.H / TAPES.CPP): the collectable video tapes (secret tape, trick tips), a CBody.
   Retail layout = the demo's shifted +0x2c. Vtable 0x800a3060. See build/hdr_notes/tapes.md. */
#ifndef GAME_TAPES_H
#define GAME_TAPES_H

#ifndef GAME_DATA_TU /* (the data files: full debug types, for data_image.py) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"

class CGlow;

struct STapeMessage {
	int essential; /* 0x0 */
	char *Line1;   /* 0x4 */
	char *Line2;   /* 0x8 */
	char *Line3;   /* 0xc */
};

class CTape : public CBody {
  public:
	CGlow *mpGlow;               /* 0x134 */
	ushort mGlowRadiusOrg;       /* 0x138 */
	ushort mGlowRadiusAmp;       /* 0x13a */
	STapeMessage *mMessageTable; /* 0x13c */
	int mSessionTapeNum;         /* 0x140 */
	int mTapeNum;                /* 0x144 */
	int mTextX;                  /* 0x148 */
	int mTextY;                  /* 0x14c */
	int mSize;                   /* 0x150 */
	int mSize1;                  /* 0x154 */
	int mSize2;                  /* 0x158 */
	CVector mVec1;               /* 0x15c */
	CVector mVec2;               /* 0x168 */
	uchar mHasNode;              /* 0x174 */
	uchar mIs3d;                 /* 0x175 */
	uchar mDropping;             /* 0x176 */
	uchar mDropForever;          /* 0x177 */
	uchar mHoverHeight;          /* 0x178 */
	char pad179[0x17a - 0x179];
	ushort mNodeIndex;          /* 0x17a */
	int mGroundY;               /* 0x17c */
	CVector mOrgPos;            /* 0x180 */
	ushort mT;                  /* 0x18c */
	ushort mMaxWobbleAmplitude; /* 0x18e */
	ushort mWobbleAmplitude;    /* 0x190 */
	ushort mWobbleSpeed;        /* 0x192 */
	ushort mLifetime;           /* 0x194 */
	char pad196[0x198 - 0x196];
	uint mTimer; /* 0x198 */
	/* sizeof(CTape) == 0x19c */

	static int mOkayToDisplayTitle;
	static int mOkayToDisplayContinueButton;

	static int DisplayTitle();          /* 0x80072ec8 */
	static int DisplayContinueButton(); /* 0x80072ed4 */
	CTape(ushort, const CVector &, const CVector &, int, int, const CVector &, uint,
	      int);                 /* 0x80072ee0 */
	virtual ~CTape();           /* slot 0: 0x80073228 */
	void DontDisplay();         /* 0x80073294 */
	void CreateBit();           /* 0x800732d0 */
	virtual void DeleteStuff(); /* slot 6: 0x800732e4 */
	void DoPhysics();           /* 0x8007333c */
	void AI(int);               /* 0x800733ec (not the virtual slot 2) */
};

#endif
