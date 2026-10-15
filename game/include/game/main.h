/* The main loop (MAIN.CPP / PLAYAWAY): per-frame display, the FPS counter, game over, creating
   the skaters and cameras. */
#ifndef GAME_MAIN_H
#define GAME_MAIN_H

#include "game/types.h"
class CBody;
class CBruce;
class CPowerUp;

/* functions */
void ResetMainDemoTimer();
void CalcPolyBufferEnd();
void PollHost();
void Game_Over();
void Game_Logic();
void Display();
void FPS_Init();
void FPS_Display();
void PlayAway_Init();
void CreateSkatersAndCameras();
void PlayAway();
int LaunchGame();

/* globals */
extern int DAT_800d1798;
extern CBruce *DAT_800d17cc;
extern int EndCode;
extern int GClear1;
extern int GClear2;
extern CPowerUp *GPowA;
extern CPowerUp *GPowE;
extern CPowerUp *GPowK;
extern CPowerUp *GPowS;
extern CPowerUp *GPowT;
extern CPowerUp *GPowTape;
extern int GSkaterRegion;
extern int GSpin;
extern short M3d_OTPushback;
extern CBody *PowerUpList;
extern int VblankUpdateBar;
#endif
