/* MECH.H: the skater object, CBruce (Neversoft "Bruce", a CSuper): construction and destruction,
   stats, glow and dynamic lighting, shadow, blood, water and its AI (MECH.CPP). */
#ifndef GAME_MECH_H
#define GAME_MECH_H

#include "game/types.h"
#include "game/vector.h"

class CBruce;

/* functions */
void SFX_Grunt(CBruce *p);

/* globals */
extern int CurrentWaterLevel2;
extern int NumMechs;

#endif
