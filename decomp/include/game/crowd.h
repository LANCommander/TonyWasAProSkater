/* Crowd reactions in competitions (CROWD.CPP): cheers, "oww"s and "ahh"s scaled by score
   and speed. */
#ifndef GAME_CROWD_H
#define GAME_CROWD_H

#include "game/types.h"

/* functions */
void Crowd_Init();           /* 0x800109b0 */
void Crowd_Start();          /* 0x800109b8 */
void Crowd_Stop();           /* 0x80010ac0 */
void Crowd_Update();         /* 0x80010bb8 */
void Crowd_Cheer(int Score); /* 0x80010d84 */
void Crowd_Oww(int Speed);   /* 0x80010eb8 */
void Crowd_Ahh(int Score);   /* 0x80010f70 */

#endif
