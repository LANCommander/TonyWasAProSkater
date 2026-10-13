/* Db_Vblank__Fv (0x80048ed8) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Vblank; THPS1 moved the demo's second loop into Db_DrawPending.
   Flips any buffer that finished drawing. The unused frame delta (a dead read of the volatile
   vblank counter, demo line 209) is what keeps &LastVblanks out of the loop's giv set. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/db.h"
#include "game/m3d.h"

extern SDoubleBuffer DoubleBuffer[2];
extern volatile int Vblanks;

void Db_Vblank() {
	int i;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].FlipNow) {
			int frames = Vblanks - DoubleBuffer[i].LastVblanks;

			PutDispEnv(&DoubleBuffer[1 - i].Disp);

			DoubleBuffer[i].FlipNow = 0;
			DoubleBuffer[i].Visible = 1;
			DoubleBuffer[1 - i].Visible = 0;
			DoubleBuffer[i].LastVblanks = Vblanks;
		}
	}

	Db_DrawPending();
}
