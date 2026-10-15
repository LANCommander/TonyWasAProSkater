/* Palette (CLUT) loading (PAL.CPP). */
#ifndef GAME_PAL_H
#define GAME_PAL_H

#include "game/types.h"

/* functions */
struct tag_S_Pal;
void Pal_Init();
tag_S_Pal *Pal_FindPaletteEntry(uint id);
void Pal_RemoveUnusedPalettes();
int GetFree16Slot();
void Free16Slot(unsigned short clut);
int GetFree256Slot();
void Free256Slot(unsigned short clut);
int Get256SlotNumber(unsigned short clut);

tag_S_Pal *Pal_LoadPalette(uint checksum, uint *pData, uchar flags);

/* globals */
extern int Pal16X;
extern int Pal16Y;
extern int Pal256X;
extern int Pal256Y;
#endif
