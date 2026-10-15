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

	CPlatform(short *p_info, int node);
	virtual ~CPlatform();
	void Bounce(CBruce *pSkater, const CVector &Direction);
	void BouncyAI();
	virtual void AI();
	virtual int Hit(int, const CVector &, int);
	void DoPhysics();
	void MoveTo(CVector &pos);
	virtual int ExecuteCommand(ushort command);
	virtual void SetVariable(ushort variable);
	virtual short GetVariable(ushort variable);
};

/* functions */
void Platform_HandleBounce();
#endif
