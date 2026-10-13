/* The main loop (MAIN.CPP / PLAYAWAY): per-frame display, the FPS counter, game over, creating
   the skaters and cameras. */
#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "game/types.h"

/* functions */
void ResetMainDemoTimer();      /* 0x800241d4 */
void CalcPolyBufferEnd();       /* 0x80024258 */
void PollHost();                /* 0x800242cc */
void Game_Over();               /* 0x800242d4 */
void Game_Logic();              /* 0x80024868 */
void Display();                 /* 0x80024fa4 */
void FPS_Init();                /* 0x80025400 */
void FPS_Display();             /* 0x80025440 */
void PlayAway_Init();           /* 0x8002566c */
void CreateSkatersAndCameras(); /* 0x80025a88 */
void PlayAway();                /* 0x80025e00 */

#endif
