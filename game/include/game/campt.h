/* CAMPT.H: camera points: creating them and choosing the one closest to the skater (CAMPT.CPP). */
#ifndef GAME_CAMPT_H
#define GAME_CAMPT_H

#include "game/types.h"
#include "game/vector.h"

class CBruce;

/* functions */
void FUN_8004e1d4();
void FUN_8004e1e0();
void CamPt_New(int n);
void CamPt_SetClosestCamera(CBruce *pSkater);

/* globals */
extern int CamPtIndex;
extern int LastDpqMax;
extern int LastDpqMin;

#endif
