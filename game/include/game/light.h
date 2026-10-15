/* LIGHT.H: CLight, a dynamic light (LIGHT.CPP). */
#ifndef GAME_LIGHT_H
#define GAME_LIGHT_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/bit.h"

/* a dynamic light (mType 2; mLifetime holds its trigger node); vtable 0x8009f0b0 */
class CLight : public CNonRenderedBit {
  public:
	int mInnerRadius;  /* 0x48 */
	int mFringeRadius; /* 0x4c */
	uchar mr0;         /* 0x50 */
	uchar mg0;         /* 0x51 */
	uchar mb0;         /* 0x52 */
	uchar mr1;         /* 0x53 */
	uchar mg1;         /* 0x54 */
	uchar mb1;         /* 0x55 */
	uchar mOn;         /* 0x56 */
	char pad57[0x58 - 0x57];
	int mAmp;            /* 0x58 */
	int mClipDist;       /* 0x5c */
	CWobblyGlow *mpGlow; /* 0x60 */
	/* sizeof(CLight) == 0x64 */

	CLight(const CVector &Pos, int Node, int ClipDist, int InnerRadius, int FringeRadius, int Amp,
	       uchar r0, uchar g0, uchar b0, uchar r1, uchar g1, uchar b1);
	virtual ~CLight();
	virtual void Move();
};
CHECK_CLASS_SIZE(CLight, SIZEOF_CLIGHT);

#endif
