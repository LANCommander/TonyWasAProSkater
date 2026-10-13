/* VRAM texture-page packing (PACK.CPP). */
#ifndef GAME_PACK_H
#define GAME_PACK_H

#include "game/types.h"

/* functions */
struct TSPack;

TSPack *Pack_NewFull(int, int, int, int); /* 0x800496b0 */
TSPack *Pack_Pack(int Type, int WW, int H, int *X, int *Y, int BPP, int Aligned,
                  uint checksum); /* 0x8004982c */

#endif
