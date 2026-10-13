/* Decompiled data that src/tapes/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char DeleteStuff__5CTape[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_800447ec[];
extern char dtor_5CTape[] PSX_NAME("_._5CTape");

PSX_FIXED(0x800a3060) int D_800a3060[16] = {
	0, 0, 0, (int)dtor_5CTape, 0, (int)Die__5CBody, 0, (int)FUN_800447ec, 0, (int)FUN_800447e4, 0,
	(int)FUN_800447dc, 0, (int)FUN_80043cfc, 0, (int)DeleteStuff__5CTape,
};
