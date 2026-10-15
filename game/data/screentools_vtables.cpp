/* The vtables of the classes src/screentools/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char Display__14CGouraudRibbon[];
extern char FUN_8002ee64[];
extern char dtor_14CGouraudRibbon[] PSX_NAME("_._14CGouraudRibbon");

PSX_DATA(0x8009f708) PsxVtEntry D_8009f708[4] = {
	{0, 0, 0},
	{0, 0, dtor_14CGouraudRibbon},
	{0, 0, FUN_8002ee64},
	{0, 0, Display__14CGouraudRibbon},
};
