/* M3DCOLIJ.H: line collision against the level's items: the line query setup, the item and face
   caches, per-item tests (with trigger faces firing their nodes) and line-to-sphere tests against
   bodies (M3DCOLIJ.CPP). */
#ifndef GAME_M3DCOLIJ_H
#define GAME_M3DCOLIJ_H

#include "game/types.h"
#include "game/vector.h"

class CBody;
struct Item;
struct SLineInfo;

/* functions */
void M3dColij_InitLineInfo(SLineInfo *pInfo);
void M3dColij_LineToItem(Item *it, SLineInfo *li);
int M3dColij_GetLineInfo(SLineInfo *li);
CBody *M3dColij_LineToSphere(const CVector &OldPos, const CVector &NewPos, CVector &ColijPos, CBody *pBody, CBody *pIgnore, int ScaleFactor = 0);

/* globals */
extern uint *EnvModelChecksums[];
extern int OtherEnvIndex;
extern int TriggerCollisionCheck;

#endif
