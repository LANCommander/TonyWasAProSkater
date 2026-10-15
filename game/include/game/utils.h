/* Odds and ends of UTILS.CPP and DB.CPP's render call: ground height, the explosion stack,
   pausing. */
#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
void Pause(int n);
void Utils_InitialRand(int seed);
int Rnd(int n);
void Utils_RotateY(CVector &out, const CVector &in, int a);
void Utils_CalcUnit(CVector &Unit, const CVector &Vec);
void Utils_GetVecFromMagDir(CVector &out, int mag, const CSVector &dir);
int Utils_Dist(const CVector &a, const CVector &b);
int Utils_XZDist(const CVector &a, const CVector &b);
int Utils_CrapDist(const CVector &a, const CVector &b);
int Utils_CalcAim(CSVector &o, const CVector &a, const CVector &b);
void Utils_TurnTowards(CSVector a, CSVector &o1, CSVector &o2, CSVector b, int speed);
void Utils_ClearExplosionStack();
int Utils_CompareStrings(const char *a, const char *b, unsigned n);
void Utils_CopyString(const char *s, char *d);
int Utils_ArcCos(int a);
int Utils_LineOfSight(const CVector &a, const CVector &b);
int Utils_GetGroundHeight(const CVector &p, int up, int down);
int Utils_KillEverythingInBox(const CVector &a, const CVector &b);
void Utils_SetVisibilityInBox(const CVector &a, const CVector &b, bool arg2, bool arg3);
int Utils_CalculateSpatialAttenuation(const CVector &pos, int near, int far);

#endif
