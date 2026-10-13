/* Decompiled data that src/powerup/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char DeleteStuff__8CPowerUp[];
extern char Die__8CPowerUp[];
extern char FUN_800206ac[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char dtor_8CPowerUp[] PSX_NAME("_._8CPowerUp");

PSX_FIXED(0x8009f0f8) int __vt_8CPowerUp[16] = {
	0, 0, 0, (int)dtor_8CPowerUp, 0, (int)Die__8CPowerUp, 0, (int)FUN_800206ac, 0,
	(int)FUN_800447e4, 0, (int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)DeleteStuff__8CPowerUp,
};
