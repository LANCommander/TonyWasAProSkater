/* Decompiled data that src/smoke/ uses and that stays at its PS1 address (PSX_FIXED, psxdata.h):
 * vtables (the port stores vptrs as numbers) and DoubleBuffer (GPU packets, 24-bit links). */
#include "psxdata.h"

extern char Display__15CTexturedRibbon[];
extern char Move__9CSmokeJet[];
extern char dtor_9CSmokeJet[] PSX_NAME("_._9CSmokeJet");

PSX_FIXED(0x8009e2c0) int D_8009e2c0[8] = {
	0, 0, 0, (int)dtor_9CSmokeJet, 0, (int)Move__9CSmokeJet, 0, (int)Display__15CTexturedRibbon,
};
