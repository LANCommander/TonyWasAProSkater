/* The vtables of the classes src/tapes/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char DeleteStuff__5CTape[];
extern char Die__5CBody[];
extern char FUN_80043cfc[];
extern char FUN_800447dc[];
extern char FUN_800447e4[];
extern char FUN_800447ec[];
extern char dtor_5CTape[] PSX_NAME("_._5CTape");

PSX_DATA(0x800a3060) PsxVtEntry D_800a3060[8] = {
	{0, 0, 0},
	{0, 0, dtor_5CTape},
	{0, 0, Die__5CBody},
	{0, 0, FUN_800447ec},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800447dc},
	{0, 0, FUN_80043cfc},
	{0, 0, DeleteStuff__5CTape},
};
