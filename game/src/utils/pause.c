/* Pause__Fi (0x800478f4) */
#include "pc_frame.h"

volatile unsigned Vblanks;

void Pause(int n) {
	unsigned t = Vblanks + n;

	/* PC: no interrupt counts the frames while this spins: wait for them (pc_frame.h) */
	while (Vblanks < t)
		PC_FrameWait(1);
}
