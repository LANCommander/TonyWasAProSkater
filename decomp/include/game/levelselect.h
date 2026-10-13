/* THPS1's career level-select screen (after PRE.CPP; no demo counterpart): level pictures
   streamed into pack slots, greyed out when locked. */
#ifndef GAME_LEVELSELECT_H
#define GAME_LEVELSELECT_H

#include "game/types.h"

/* functions */
int LevelSelect_Run(int skater);           /* 0x8006658c */
void MakeGreyClut(int x, int y);           /* 0x800665ec */
void LevelSelect_StartPicLoad(int first);  /* 0x800666f4 */
void LevelSelect_ContinueLoading();        /* 0x80066768 */
void LevelSelect_DecodePics();             /* 0x800668e8 */
void LevelSelect_InitPackSlots();          /* 0x80066aa8 */
void LevelSelect_SelectPackSlot(int slot); /* 0x80066bb4 */
void LevelSelect_FreePackSlots();          /* 0x80066c70 */
void LevelSelect_Screen();                 /* 0x80066d7c */

#endif
