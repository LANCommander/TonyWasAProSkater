/* BIT2.H: the line bits (CPolyLine, CGPolyLine, CGLine), the grind sparks and the smoke
   generator (BIT2.CPP). */
#ifndef GAME_BIT2_H
#define GAME_BIT2_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/bit.h"

struct SLineSeg;

/* vtable 0x8009f670 */
class CPolyLine : public CBit {
  public:
	uchar mForcedPriority; /* 0x48 */
	char pad49[0x4c - 0x49];
	int mNumSegs;    /* 0x4c */
	SLineSeg *mSegs; /* 0x50 */
	CVector mStart;  /* 0x54 */
	/* sizeof(CPolyLine) == 0x60 */

	CPolyLine(int); /* FUN_800270e0; demo __9CPolyLinei */
	virtual ~CPolyLine();
	void SetSemiTransparent();
};

/* vtable 0x8009f658 */
class CGPolyLine : public CBit {
  public:
	int mNumSegs;          /* 0x48 */
	SLineSeg *mSegs;       /* 0x4c */
	CVector mStart;        /* 0x50 */
	uchar mStartR;         /* 0x5c */
	uchar mStartG;         /* 0x5d */
	uchar mStartB;         /* 0x5e */
	uchar mCode;           /* 0x5f */
	uchar mForcedPriority; /* 0x60 */
	char pad61[0x64 - 0x61];
	/* sizeof(CGPolyLine) == 0x64 */

	CGPolyLine(int n);
	virtual ~CGPolyLine();
	void SetSemiTransparent();
};

/* vtable 0x8009f628 */
class CGLine : public CBit {
  public:
	uint mCodeBGR0; /* 0x48 */
	uint mPadBGR1;  /* 0x4c */
	CVector mStart; /* 0x50 */
	CVector mEnd;   /* 0x5c */
	/* sizeof(CGLine) == 0x68 */

	CGLine();
	virtual ~CGLine();
};

/* vtable 0x8009f610 */
class CGrindSpark : public CGLine {
  public:
	CVector mOldPos; /* 0x68 */
	CBody *mpParent; /* 0x74 */
	/* sizeof(CGrindSpark) == 0x78 */

	CGrindSpark(CBody *pParent, CVector &pos, int vel, int gravity, int life);
	virtual ~CGrindSpark();
	virtual void Move();
};

/* emits CMotionBlur smoke puffs; vtable 0x8009f640 */
class CSmokeGenerator : public CNonRenderedBit {
  public:
	int mPuffs; /* 0x48 */
	uchar mR;   /* 0x4c */
	uchar mG;   /* 0x4d */
	uchar mB;   /* 0x4e */
	char pad4f[0x50 - 0x4f];
	int mVBase;       /* 0x50 */
	int mVRandom;     /* 0x54 */
	int mScaleBase;   /* 0x58 */
	int mScaleRandom; /* 0x5c */
	/* sizeof(CSmokeGenerator) == 0x60 */

	virtual ~CSmokeGenerator();
	virtual void Move();
};

#endif
