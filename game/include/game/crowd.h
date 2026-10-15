/* Crowd reactions in competitions (CROWD.CPP): cheers, "oww"s and "ahh"s scaled by score
   and speed. */
#ifndef GAME_CROWD_H
#define GAME_CROWD_H

#include "game/types.h"

/* functions */
void Crowd_Init();
void Crowd_Start();
void Crowd_Stop();
void Crowd_Update();
void Crowd_Cheer(int a);
void Crowd_Oww(int Speed);
void Crowd_Ahh(int a);

#endif
