/* M3DINIT.H: renderer setup: the resolution, the fog parameters and parsing a level's PSX file
   (M3DINIT.CPP). */
#ifndef GAME_M3DINIT_H
#define GAME_M3DINIT_H

#include "game/types.h"
#include "game/vector.h"

/* functions */
void M3dInit_SetResolution(unsigned w, unsigned h);
void M3dInit_SetFoggingParams(int a, int nearz, unsigned range);
void M3dInit_ParsePSX(int Region);

#endif
