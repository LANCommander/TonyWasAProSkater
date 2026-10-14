/* FUN_80048d00 (0x80048d00) */
#include "game/dbuffer.h"

/* Both display buffers back at the default screen position. */
void FUN_80048d00(void) {
	DoubleBuffer[1].Disp.screen.x = 0;
	DoubleBuffer[0].Disp.screen.x = 0;
	DoubleBuffer[1].Disp.screen.y = 0;
	DoubleBuffer[0].Disp.screen.y = 0;
}
