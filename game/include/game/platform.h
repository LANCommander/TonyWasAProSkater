/* CPlatform (PLATFORM.H / PLATFORM.CPP): moving / bouncy / scalable script platforms, a CBaddy.
   Retail layout = the demo's shifted +8 (after CBaddy). Vtable 0x800a013c. See
   build/hdr_notes/platform.md. */
#ifndef GAME_PLATFORM_H
#define GAME_PLATFORM_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"
#include "game/baddy.h"

class CBruce;

class CPlatform : public CBaddy {
  public:
	ushort mScaleXTimer;      /* 0x21c */
	ushort mScaleYTimer;      /* 0x21e */
	ushort mScaleZTimer;      /* 0x220 */
	ushort mScaleXAdjust;     /* 0x222 */
	ushort mScaleYAdjust;     /* 0x224 */
	ushort mScaleZAdjust;     /* 0x226 */
	short mBruceHealthAdjust; /* 0x228 */
	ushort mNumBounces;       /* 0x22a */
	ushort mType;             /* 0x22c (hides CItem::mType) */
	ushort mWidth;            /* 0x22e */
	ushort mHeight;           /* 0x230 */
	ushort mDepth;            /* 0x232 */
	ushort mDeathTimer;       /* 0x234 */
	CSVector mTargetAngles;   /* 0x236 */
	uint mTrodUpon;           /* 0x23c */
	uint mNotTrodUponTimer;   /* 0x240 */
	uint mLoopingSound;       /* 0x244 (hides CBaddy's) */
	int mLoopingSoundUpdate;  /* 0x248 (hides CBaddy's) */
	int mSpin[3];             /* 0x24c (demo: CVector mBoundingBox) */
	/* sizeof(CPlatform) == 0x258 */

	CPlatform(short *, int);                    /* 0x8003d6d8 */
	virtual ~CPlatform();                       /* slot 0: 0x8003d7a0 */
	void Bounce(CBruce *, const CVector &);     /* 0x8003d820 */
	void BouncyAI();                            /* 0x8003db00 */
	virtual void AI();                          /* slot 2: 0x8003e390 */
	virtual int Hit(int, const CVector &, int); /* slot 5: 0x8003e934 */
	void DoPhysics();                           /* 0x8003e9d8 */
	void MoveTo(CVector &);                     /* 0x8003ebdc */
	virtual int ExecuteCommand(ushort);         /* slot 7: 0x8003ec48 */
	virtual void SetVariable(ushort);           /* slot 8: 0x8003f45c */
	virtual short GetVariable(ushort);          /* slot 9: 0x8003f624 */
};

/* functions */
void Platform_HandleBounce(); /* 0x8003f65c */

#endif
