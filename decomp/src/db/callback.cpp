/* Db_CallBack__Fv (0x80048e40) */
/* C++ (DB.CPP). Demo twin; locals from its MAIN.SYM. Draw-sync callback for the two display
   buffers: once the GPU is idle, any buffer waiting to be drawn is marked done. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SDoubleBuffer DoubleBuffer[2];

void Db_CallBack() {
	if (!DoubleBuffer[0].Rendering && !DoubleBuffer[1].Rendering)
		return;

	if (!DrawSync(1)) {
		int i;

		for (i = 0; i < 2; i++) {
			if (DoubleBuffer[i].Rendering) {
				DoubleBuffer[i].Rendering = 0;
				DoubleBuffer[i].FlipNow = 1;
			}
		}
	}
}
