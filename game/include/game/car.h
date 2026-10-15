/* CCar (CAR.H / CAR.CPP): road traffic, a CBaddy following road segments and honking at skaters.
   Retail layout = the demo's shifted +8 (after CBaddy). Vtable 0x800a233c. See
   build/hdr_notes/car.md. */
#ifndef GAME_CAR_H
#define GAME_CAR_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/ob.h"
#include "game/baddy.h"

class CCar : public CBaddy {
  public:
	int mCurrentRoadSegment; /* 0x21c */
	int mHonkLock;           /* 0x220 */
	/* sizeof(CCar) == 0x224 */

	CCar(short *p_info, int node, int type);
	virtual ~CCar();
	virtual void AI();
	virtual int ExecuteCommand(ushort); /* returns 1 */
};

#endif
