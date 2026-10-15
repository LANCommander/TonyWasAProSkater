/* The vtables of the classes src/quat/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char FUN_80078088[];
extern char FUN_800780bc[];

PSX_DATA(0x800a3474) PsxVtEntry D_800a3474[2] = {
	{0, 0, 0},
	{0, 0, FUN_80078088},
};

PSX_DATA(0x800a3484) PsxVtEntry D_800a3484[2] = {
	{0, 0, 0},
	{0, 0, FUN_800780bc},
};
