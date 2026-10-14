/* Palette (CLUT) loading (PAL.CPP). */
#ifndef GAME_PAL_H
#define GAME_PAL_H

#include "game/types.h"

/* functions */
struct tag_S_Pal;

tag_S_Pal *Pal_LoadPalette(uint checksum, uint *pData, uchar flags); /* 0x8004aea4 */

#endif
