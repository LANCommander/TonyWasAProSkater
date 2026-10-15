/* Game initialisation and cleanup (INIT.CPP). */
#ifndef GAME_INIT_H
#define GAME_INIT_H

#include "game/types.h"

/* functions */
void Init_Cleanup(int a);
void Init_Restart(int type, int b);
void Init_ForGame();
void FUN_80071f84();
void Init_ResetMemory(int a);

#endif
