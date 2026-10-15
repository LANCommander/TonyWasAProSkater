/* The vtables of the classes src/shellutils/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char dtor_12VerticalMenu[] PSX_NAME("_._12VerticalMenu");
extern char draw__12VerticalMenu[];

PsxVtEntry __vt_12VerticalMenu[3] = {
	{0, 0, 0},
	{0, 0, dtor_12VerticalMenu},
	{0, 0, draw__12VerticalMenu},
};
