/* The vtables of the classes src/ob/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_800447ec[];
extern char dtor_5CBody[] PSX_NAME("_._5CBody");
extern char dtor_6CSuper[] PSX_NAME("_._6CSuper");

PsxVtEntry __vt_6CSuper[8] = {
	{0, 0, 0},
	{0, 0, dtor_6CSuper},
	{0, 0, Die__5CBody},
	{0, 0, FUN_800447ec},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
};

PsxVtEntry __vt_5CBody[8] = {
	{0, 0, 0},
	{0, 0, dtor_5CBody},
	{0, 0, Die__5CBody},
	{0, 0, FUN_800447ec},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
};
