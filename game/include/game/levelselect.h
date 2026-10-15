/* THPS1's career level-select screen (after PRE.CPP; no demo counterpart): level pictures
   streamed into pack slots, greyed out when locked. */
#ifndef GAME_LEVELSELECT_H
#define GAME_LEVELSELECT_H

#include "game/types.h"

/* functions */
int LevelSelect_Run(int a);
void MakeGreyClut(int x, int y);
void LevelSelect_StartPicLoad(int a);
void LevelSelect_ContinueLoading();
void LevelSelect_DecodePics();
void LevelSelect_InitPackSlots();
void LevelSelect_SelectPackSlot(int n);
void LevelSelect_FreePackSlots();
void LevelSelect_Screen();

#endif
