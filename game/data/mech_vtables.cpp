/* The vtables of the classes src/mech/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char Die__6CBruce[];
extern char FUN_80042380[];
extern char FUN_800425b8[];
extern char FUN_800427bc[];
extern char FUN_800427c4[];
extern char FUN_80042d04[];
extern char FUN_800447e4[];

PSX_DATA(0x800a0534) PsxVtEntry DAT_800a0534[8] = {
	{0, 0, 0},
	{0, 0, FUN_80042380},
	{0, 0, Die__6CBruce},
	{0, 0, FUN_80042d04},
	{0, 0, FUN_800447e4},
	{0, 0, FUN_800425b8},
	{0, 0, FUN_800427c4},
	{0, 0, FUN_800427bc},
};
