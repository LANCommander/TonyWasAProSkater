/* vWibble__FP15STexWibVertInfo (0x8008523c) */
/* C++ (M3D.CPP, between M3d_BuildTransform and M3d_PreprocessWibblyTextures). Looks up a face's
   wibble offset: the high nibble of byte 3 picks a 64-entry row of a short table (0 = none), the
   low nibble phase-shifts the global counter BasePhase into it. */
#include "game/m3d.h"

extern int BasePhase;

extern short WibbleTables[][64];

int vWibble(STexWibVertInfo *p) {
	int hi = p->vAmpPhase >> 4;
	int lo = p->vAmpPhase & 15;

	if (hi) {
		int i = (BasePhase + lo * 4) & 63;
		short *row = WibbleTables[hi];

		return row[i];
	}

	return 0;
}
