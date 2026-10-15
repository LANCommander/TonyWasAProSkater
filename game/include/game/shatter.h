/* SHATTER.H: breakable glass and shattering items (SHATTER.CPP). */
#ifndef GAME_SHATTER_H
#define GAME_SHATTER_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/bit.h"
#include "game/types.h"
struct Item;

/* a falling shard of a shattered item (SHATTER.CPP); vtable 0x8009f280 */
class CShatterBit : public CChunkBit {
  public:
	CSVector mAngVel; /* 0xc0 */
	char padc6[0xc8 - 0xc6];
	CSmokeTrail *mpSmokeTrail; /* 0xc8 */
	/* sizeof(CShatterBit) == 0xcc */

	CShatterBit(const CSVector &a, const CSVector &b, const CSVector &c, const CVector &pos,
	            int Smoke);
	virtual ~CShatterBit();
	void SetPos(const CVector &Pos);
	virtual void Move();
};

/* functions */
int Shatter_Face(Item *pItem, uint *pFace, int NumSplits, int MakeFlash, int UseFlags, int ModifyFlags, int TryToShatter);
int Shatter_Item(Item *it, int a, int b);
void Shatter_MaybeMakeGlassShatterSound();

#endif
