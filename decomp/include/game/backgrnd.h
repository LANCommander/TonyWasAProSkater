/* CBackground (BACKGRND.H / BACKGRND.CPP): a spinning background model, a CBody (retail and demo
   alike: its Die slot is CBody::Die). Vtable 0x800a3494. See build/hdr_notes/backgrnd.md. */
#ifndef GAME_BACKGRND_H
#define GAME_BACKGRND_H

#pragma interface /* see class.h */

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"

class CBackground : public CBody {
  public:
	ushort mIndex;                 /* 0x134 */
	char pad136[0x138 - 0x136];
	/* sizeof(CBackground) == 0x138 */

	CBackground(uint, const CSVector &);          /* 0x80078188 */
	virtual ~CBackground();                       /* slot 0: 0x8007821c */
	virtual void AI();                            /* slot 2: 0x8007828c */
};

#endif
