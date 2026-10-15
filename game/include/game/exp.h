/* EXP.H: explosions hitting the environment and the water ripple CFlatRipple (EXP.CPP). */
#ifndef GAME_EXP_H
#define GAME_EXP_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/bit2.h"
#include "game/types.h"
struct Item;

/* an expanding ring on water (EXP.CPP); vtable 0x800a1308 */
class CFlatRipple : public CPolyLine {
  public:
	uchar mR; /* 0x60 */
	uchar mG; /* 0x61 */
	uchar mB; /* 0x62 */
	char pad63[0x64 - 0x63];
	int mMaxRadius; /* 0x64 */
	int mRadius;    /* 0x68 */
	int mGrowRate;  /* 0x6c */
	int mAngle;     /* 0x70 */
	/* sizeof(CFlatRipple) == 0x74 */

	CFlatRipple(const CVector &Pos, int MaxRadius, uchar r, uchar g, uchar b, int NumSegs,
	            int GrowRate);
	virtual ~CFlatRipple();
	virtual void Move();
};

/* functions */
void Exp_HitEnvItem(Item *pItem, uint *pFace, int Damage);
void FUN_8004ee24();
#endif
