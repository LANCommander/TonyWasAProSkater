/* The vtables of the classes src/backgrnd/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char AI__11CBackground[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char dtor_11CBackground[] PSX_NAME("_._11CBackground");

PsxVtEntry __vt_11CBackground[8] = {
	{0, 0, 0},
	{0, 0, dtor_11CBackground},
	{0, 0, Die__5CBody},
	{0, 0, AI__11CBackground},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
};
