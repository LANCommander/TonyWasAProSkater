/* Pause__Fi (0x800478f4) */
#ifdef PC_PORT
#include "pc_frame.h"
#endif

volatile unsigned Vblanks;

void Pause(int n) {
	unsigned t = Vblanks + n;

#ifdef PC_PORT
	/* PC: no interrupt counts the frames while this spins: wait for them (pc_frame.h) */
	while (Vblanks < t)
		PC_FrameWait(1);
#else
	while (Vblanks < t)
		;
#endif
}
