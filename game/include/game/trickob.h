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

	CTrickOb(unsigned short Node);
	virtual ~CTrickOb();
};
CHECK_CLASS_SIZE(CTrickOb, SIZEOF_CTRICKOB);

/* functions */
void TrickOb_DeleteAll();
int TrickOb_TrickedOnItem(Item *a, void *pb, int c);
void TrickOb_DarkenNode(int id);
int TrickOb_GetItemType(Item *o);
void TrickOb_Update();

/* globals */
/* the last tagging skater (uchar) and its trick score (ushort) */
extern int TrickPlayer;
#endif
