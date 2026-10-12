/* Db_Vblank__Fv (0x80048ed8) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Vblank; THPS1 moved the demo's second loop into FUN_80048d24.
   Flips any buffer that finished drawing. The unused frame delta (a dead read of the volatile
   vblank counter, demo line 209) is what keeps &LastVblanks out of the loop's giv set. */
#include "game/m3d.h"

extern SDoubleBuffer DoubleBuffer[2];
extern volatile int Vblanks;

extern "C" int FUN_8008ca0c(DISPENV *);
extern "C" void FUN_80048d24(void);

void Db_Vblank() {
	int i;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].FlipNow) {
			int frames = Vblanks - DoubleBuffer[i].LastVblanks;

			FUN_8008ca0c(&DoubleBuffer[1 - i].Disp);

			DoubleBuffer[i].FlipNow = 0;
			DoubleBuffer[i].Visible = 1;
			DoubleBuffer[1 - i].Visible = 0;
			DoubleBuffer[i].LastVblanks = Vblanks;
		}
	}

	FUN_80048d24();
}
