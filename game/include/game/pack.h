/* VRAM texture-page packing (PACK.CPP). */
#ifndef GAME_PACK_H
#define GAME_PACK_H

#include "game/types.h"

/* functions */
struct TSPack;
void Pack_KillSlot(TSPack *p);
void Pack_Init();
void Pack_Init640x480();
TSPack *Pack_Pack(int Type, int WW, int H, int *X, int *Y, int BPP, int Aligned, uint checksum);
void Pack_Unpack(short x, short y);
void Pack_DefragmentFully();

TSPack *Pack_NewFull(int a, int b, int c, int d);

/* globals */
/* the VRAM pack lists (game/m3d.h TSPack) */
extern TSPack *Pack_pFree;
extern TSPack *Pack_pFull;
#endif
