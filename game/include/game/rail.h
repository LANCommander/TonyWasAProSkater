/* RAIL.H: the grind rails, CRail, node runs with bounding boxes on RailList, and the skaters' rail
   pickup (RAIL.CPP). */
#ifndef GAME_RAIL_H
#define GAME_RAIL_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/classsizes.h"
#include "game/class.h"

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

	virtual ~CRail();
};
CHECK_CLASS_SIZE(CRail, SIZEOF_CRAIL);

/* functions */
void FUN_8004f5d0();
void Rail_DeleteAll();
CRail *FUN_8004f684(CRail *p, int idx);
int Rail_NextNode(unsigned short *p);
void Rail_Update();

#endif
