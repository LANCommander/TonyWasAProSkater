/* The vtables of the classes src/powerup/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char DeleteStuff__8CPowerUp[];
extern char Die__8CPowerUp[];
extern char FUN_800206ac[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char dtor_8CPowerUp[] PSX_NAME("_._8CPowerUp");

PSX_DATA(0x8009f0f8) PsxVtEntry __vt_8CPowerUp[8] = {
	{0, 0, 0},
	{0, 0, dtor_8CPowerUp},
	{0, 0, Die__8CPowerUp},
	{0, 0, FUN_800206ac},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, DeleteStuff__8CPowerUp},
};
