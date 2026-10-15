/* The vtables of the classes src/smoke/ defines (native data, psxdata.h). */
#include "psxdata.h"

extern char Display__15CTexturedRibbon[];
extern char Move__9CSmokeJet[];
extern char dtor_9CSmokeJet[] PSX_NAME("_._9CSmokeJet");

PSX_DATA(0x8009e2c0) PsxVtEntry D_8009e2c0[4] = {
	{0, 0, 0},
	{0, 0, dtor_9CSmokeJet},
	{0, 0, Move__9CSmokeJet},
	{0, 0, Display__15CTexturedRibbon},
};
