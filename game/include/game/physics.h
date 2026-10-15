/* Free functions of PHYSICS.CPP (the CBruce methods are in game/bruce.h). */
#ifndef GAME_PHYSICS_H
#define GAME_PHYSICS_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
struct CVector;
void SetGravity(CVector *v);
int arccos(int a);

/* globals */
extern int BitCount;
extern int BulletCollisionCheck;
extern int Cheat_Fire;
extern int Cheat_StatsOverride;
extern int Face_Terrain;
extern int GHorsePlayer;
extern int TimeScale;
extern int TimeScaleSquared;
extern int XFrames;
#endif
