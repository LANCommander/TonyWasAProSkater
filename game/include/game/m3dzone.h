/* M3DZONE.H: the level's zones: freeing a level's zone data, line collision against the items of a
   zone, and selecting the current zone (M3DZONE.CPP). */
#ifndef GAME_M3DZONE_H
#define GAME_M3DZONE_H

#include "game/types.h"
#include "game/vector.h"

struct SLineInfo;

/* functions */
void FUN_800827ac();
void M3dZone_SetZone(int EnvIndex, uint *pPack);
void M3dZone_FreePSX(int a);
int M3dZone_LineToItemNotSCached(SLineInfo *pInfo, int CheckEnviroObs);

/* globals */
extern int BaddyCollisionCheck;
extern int LineOfSightCheck;

#endif
