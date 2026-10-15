/* PREManager (PRE.CPP): the loaded .PRE archives (up to 16, with their names). Derives from CClass
   (game/class.h); vtable 0x800a2120. Layout = demo. See build/hdr_notes/pre.md. */
#ifndef GAME_PRE_H
#define GAME_PRE_H

#ifndef GAME_DATA_TU /* (the data files: full debug types, for data_image.py) */
#pragma interface /* see class.h */
#endif

#include "game/classsizes.h"
#include "game/types.h"
#include "game/class.h"

class PREManager : public CClass {
  public:
	char *mPre[16];        /* 0x004 */
	char mPreName[16][16]; /* 0x044 */
	/* sizeof(PREManager) == 0x144 */

	PREManager();                           /* 0x8006613c (FUN_; C source) */
	virtual ~PREManager();                  /* slot 0: 0x80066180 (FUN_; C source) */
	void loadPRE(char *pre_name);           /* 0x80066214 (FUN_; spoolPRE + FileIO_Sync) */
	void spoolPRE(char *pre_name);          /* 0x8006623c */
	void unloadPRE(char *pre_name);         /* 0x80066338 */
	char *getFile(char *pName, int *pSize); /* 0x800663d8 */
	char *parsePRE(char *pPre, char *pName, int *pSize); /* 0x80066460 */
};
CHECK_CLASS_SIZE(PREManager, SIZEOF_PREMANAGER);

#endif
