/* The display double buffers (DB.CPP): ordering-table size, draw/vsync bookkeeping, init. The
   SDoubleBuffer type itself is in game/m3d.h. */
#ifndef GAME_DB_H
#define GAME_DB_H

#include "game/types.h"

/* functions */
int Db_OTSize();       /* 0x80048c74 */
void Db_DrawPending(); /* 0x80048d24 */
void Db_WaitRender();  /* 0x80048fec */
void Db_Init();        /* 0x800491e8 */

#endif
