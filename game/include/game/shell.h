/* The in-game shell (SHELL.CPP): the pause screen. */
#ifndef GAME_SHELL_H
#define GAME_SHELL_H

#include "game/types.h"

/* functions */
int Shell_DoCheatProcessing();
void Shell_PauseScreen();
void FUN_800785cc(int a);

/* globals */
extern int Cheat_SlowMo;
extern uint CheatCheck;
extern int DAT_800d1d24;
#endif
