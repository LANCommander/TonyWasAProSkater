/* The vtables of the classes src/medals/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char DeleteStuff__6CMedal[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_80073da8[];
extern char dtor_6CMedal[] PSX_NAME("_._6CMedal");

PsxVtEntry __vt_6CMedal[8] = {
	{0, 0, 0},
	{0, 0, dtor_6CMedal},
	{0, 0, Die__5CBody},
	{0, 0, FUN_80073da8},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, DeleteStuff__6CMedal},
};
