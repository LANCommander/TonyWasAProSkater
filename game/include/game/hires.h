/* The 640x480 front-end display (HIRES.CPP): double-buffered ordering tables, blanking, the
   preview-model camera. */
#ifndef GAME_HIRES_H
#define GAME_HIRES_H

#include "game/types.h"

/* functions */
struct SViewport;

void HiRes_BlankScreen();           /* 0x8006af64 */
void HiRes_Init(int blank);         /* 0x8006b01c */
SViewport *HiRes_SetupCamera();     /* 0x8006b1c8 */
void HiRes_ClearOTag();             /* 0x8006b240 */
void HiRes_DrawOTag();              /* 0x8006b26c */
void HiRes_DrawOTagNoSync();        /* 0x8006b2f4 */
void HiRes_DrawOTagAll();           /* 0x8006b374 */
void kickIt(int level, int skater); /* 0x8006b434 */

#endif
