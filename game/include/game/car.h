/* CCar (CAR.H / CAR.CPP): road traffic, a CBaddy following road segments and honking at skaters.
   Retail layout = the demo's shifted +8 (after CBaddy). Vtable 0x800a233c. See
   build/hdr_notes/car.md. */
#ifndef GAME_CAR_H
#define GAME_CAR_H

#ifndef GAME_DATA_TU /* (the data files: full debug types, for data_image.py) */
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

	CCar(short *, int, int);            /* 0x8006b490 */
	virtual ~CCar();                    /* slot 0: 0x8006b630 */
	virtual void AI();                  /* slot 2: 0x8006bf5c */
	virtual int ExecuteCommand(ushort); /* slot 7: 0x8006c470 (returns 1) */
};

#endif
