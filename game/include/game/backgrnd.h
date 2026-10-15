/* CBackground (BACKGRND.H / BACKGRND.CPP): a spinning background model, a CBody (retail and demo
   alike: its Die slot is CBody::Die). Vtable 0x800a3494. See build/hdr_notes/backgrnd.md. */
#ifndef GAME_BACKGRND_H
#define GAME_BACKGRND_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"
class CBody;

class CBackground : public CBody {
  public:
	ushort mIndex; /* 0x134 */
	char pad136[0x138 - 0x136];
	/* sizeof(CBackground) == 0x138 */

	CBackground(uint checksum, const CSVector &ang_vel);
	virtual ~CBackground();
	virtual void AI();
};

/* functions */
void Backgrnd_On(ushort id);
void Backgrnd_Off(unsigned short id);

/* globals */
extern CBody *BackgroundList;
extern char *pCurrentObjFile;
#endif
