/* The vtables of the classes src/shellutils/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char dtor_12VerticalMenu[] PSX_NAME("_._12VerticalMenu");
extern char draw__12VerticalMenu[];

PSX_DATA(0x800a2158) PsxVtEntry D_800a2158[3] = {
	{0, 0, 0},
	{0, 0, dtor_12VerticalMenu},
	{0, 0, draw__12VerticalMenu},
};
