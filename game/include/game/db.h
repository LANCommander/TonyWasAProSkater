/* The display double buffers (DB.CPP): ordering-table size, draw/vsync bookkeeping, init. The
   SDoubleBuffer type itself is in game/m3d.h. */
#ifndef GAME_DB_H
#define GAME_DB_H

#include "game/types.h"

/* functions */
int Db_OTSize();
void Db_UpdateSky();
void FUN_80048d00();
void Db_DrawPending();
void Db_Vblank();
void Db_WaitRender();
void FUN_800491a0();
void Db_Reset();
void Db_Init();
void FUN_80049474();
void FUN_800494dc();
void Db_Render();

#endif
