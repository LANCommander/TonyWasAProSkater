/* SMOKE.H: CSmokeJet, a ribbon of smoke or steam that can hurt (SMOKE.CPP). */
#ifndef GAME_SMOKE_H
#define GAME_SMOKE_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/bit.h"

/* CSmokeJet width wobble */
struct SWidthWobble {
	int TChange;     /* 0x0 */
	int TargetWidth; /* 0x4 */
	                 /* sizeof == 0x8 */
};

/* a steam/smoke jet (SMOKE.CPP); vtable 0x8009e2c0 */
class CSmokeJet : public CTexturedRibbon {
  public:
	SWidthWobble mWidthWobble[10]; /* 0x06c */
	int mOrgX[10];                 /* 0x0bc */
	int mPhase[10];                /* 0x0e4 */
	int mFreq[10];                 /* 0x10c */
	int mAmp[10];                  /* 0x134 */
	uchar mScrollSpeed;            /* 0x15c */
	uchar mDamageBaddies;          /* 0x15d */
	ushort mDamage;                /* 0x15e */
	ushort mDamageTimer;           /* 0x160 */
	CSVector mAngles;              /* 0x162 */
	int mLength;                   /* 0x168 */
	/* sizeof(CSmokeJet) == 0x16c */

	CSmokeJet(const CVector &BasePos, const CSVector &Angles, int Length, ushort Duration,
	          int Damage, int r, int g, int b, uchar ScrollSpeed, uint Checksum);
	virtual ~CSmokeJet();
	void SetPos(const CVector &pos);
	virtual void Move();
};

#endif
