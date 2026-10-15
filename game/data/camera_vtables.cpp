/* The vtables of the classes src/camera/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char AI__7CCamera[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447d4[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_8007d944[];
extern char FUN_8007d978[];
extern char dtor_7CCamera[] PSX_NAME("_._7CCamera");

PsxVtEntry __vt_7CCamera[8] = {
	{0, 0, 0},
	{0, 0, dtor_7CCamera},
	{0, 0, Die__5CBody},
	{0, 0, AI__7CCamera},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, FUN_800447d4},
};

PsxVtEntry DAT_800a35b4[2] = {
	{0, 0, 0},
	{0, 0, FUN_8007d944},
};

PsxVtEntry DAT_800a35c4[2] = {
	{0, 0, 0},
	{0, 0, FUN_8007d978},
};
