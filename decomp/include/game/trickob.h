/* Rails (RAIL.CPP) and trick objects (TRICKOB.CPP): small CClass-derived list nodes. Layouts are
   the demo's (MAIN.SYM), unchanged in retail. vptr at +0 (CClass is the polymorphic root). */
#ifndef GAME_TRICKOB_H
#define GAME_TRICKOB_H

#pragma interface /* see class.h */

#include "game/types.h"
#include "game/class.h"
#include "game/vector.h"

struct Item;

/* a grind rail: a node run with its bounding box, on the doubly linked RailList */
class CRail : public CClass {
  public:
	/* 0x00: vptr (vtable 0x800a1340) */
	short mStartNode;  /* 0x04 */
	ushort mType;      /* 0x06 */
	CVector mBBoxMin;  /* 0x08 */
	CVector mBBoxMax;  /* 0x14 */
	CRail *mpPrevious; /* 0x20 */
	CRail *mpNext;     /* 0x24 */
	/* sizeof(CRail) == 0x28 */

	virtual ~CRail(); /* slot 0: 0x8004f814 */
};

/* a scoring object (TrickObList): node + checksum, state/owner for the trick-attack modes */
class CTrickOb : public CClass {
  public:
	/* 0x00: vptr (vtable 0x800a1320) */
	uint mChecksum; /* 0x04 */
	ushort mNode;   /* 0x08 */
	uchar mState;   /* 0x0a */
	uchar mPlayer;  /* 0x0b */
	ushort mScore;  /* 0x0c */
	char pade[0x10 - 0xe];
	CTrickOb *mpNext; /* 0x10 */
	Item *mpItem;     /* 0x14 */
	/* sizeof(CTrickOb) == 0x18 */

	CTrickOb(ushort);    /* 0x8004ef08 */
	virtual ~CTrickOb(); /* slot 0: 0x8004ef80 */
};

#endif
