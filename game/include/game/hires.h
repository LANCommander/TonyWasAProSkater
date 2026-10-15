/* The 640x480 front-end display (HIRES.CPP): double-buffered ordering tables, blanking, the
   preview-model camera. */
#ifndef GAME_HIRES_H
#define GAME_HIRES_H

#include "game/types.h"

/* functions */
struct SViewport;
void HiRes_BlankScreen();
SViewport *HiRes_SetupCamera();
void HiRes_ClearOTag();
void HiRes_DrawOTag();
void FUN_8006b3f8();

void HiRes_Init(int a);
void HiRes_DrawOTagNoSync();
void HiRes_DrawOTagAll();
void kickIt(int a, int b);

#endif
