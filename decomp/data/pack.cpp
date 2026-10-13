/* Decompiled data: the retail executable's initialised globals that src/pack/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

/* THPS2 demo: TSPack* */
PSX_DATA(0x800d1a60) int Pack_pFree[1];

/* THPS2 demo: TSPack* */
PSX_DATA(0x800d1a64) int Pack_pFull[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1a68) int TotSlots[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1a74) int Pack_Width[1];

/* EPackInitType (THPS2 demo: EPackInitType) */
PSX_DATA(0x800d1a78) int LastPackInitType[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1a7c) int ZapCalls[1];
