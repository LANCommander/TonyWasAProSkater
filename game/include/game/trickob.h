/* Rails (RAIL.CPP) and trick objects (TRICKOB.CPP): small CClass-derived list nodes. Layouts are
   the demo's (MAIN.SYM), unchanged in retail. vptr at +0 (CClass is the polymorphic root). */
#ifndef GAME_TRICKOB_H
#define GAME_TRICKOB_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/classsizes.h"
#include "game/types.h"
#include "game/class.h"
#include "game/vector.h"

struct Item;

/* a grind rail: a node run with its bounding box, on the doubly linked RailList */
class CRail : public CClass {
  public:
	/* 0x00: vptr (vtable 0x800a1340) */
	int mStartNode;    /* 0x04 (retail: a word; the demo's short + ushort mType) */
	CVector mBBoxMin;  /* 0x08 */
	CVector mBBoxMax;  /* 0x14 */
	CRail *mpPrevious; /* 0x20 */
	CRail *mpNext;     /* 0x24 */
	int mNodeParam;    /* 0x28 THPS1: the start node's position word +6 (0 if -1); a grinding
	                      skater copies it to CBruce::m210 (Rail_Update) */
	/* sizeof(CRail) == 0x2c (Trig_SendPulseToNode allocates 44) */

	virtual ~CRail(); /* slot 0: 0x8004f814 */
};
CHECK_CLASS_SIZE(CRail, SIZEOF_CRAIL);

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
CHECK_CLASS_SIZE(CTrickOb, SIZEOF_CTRICKOB);

/* functions */
int TrickOb_TrickedOnItem(Item *, void *, int); /* 0x8004f000 */
int TrickOb_GetItemType(Item *pItem);           /* 0x8004f318 */
void TrickOb_Update();                          /* 0x8004f374 */
int Rail_NextNode(ushort *);                    /* 0x8004f578 */
void Rail_Update();                             /* 0x8004f8a0 */

#endif
