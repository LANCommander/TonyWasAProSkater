/* Db_UpdateSky__Fv (0x80048c8c) */
#include "names.h"
#include "game/dbuffer.h"

extern int Db_SkyColor;

/* The sky colour (-1: none) as both buffers' background fill. */
void Db_UpdateSky(void) {
	unsigned int c = Db_SkyColor;

	if (c == -1) {
		DoubleBuffer[0].Draw.isbg = 0;
		DoubleBuffer[1].Draw.isbg = 0;
	} else {
		DoubleBuffer[0].Draw.isbg = 1;
		DoubleBuffer[0].Draw.r0 = c >> 16;
		DoubleBuffer[0].Draw.g0 = c >> 8;
		DoubleBuffer[0].Draw.b0 = Db_SkyColor;
		DoubleBuffer[1].Draw.isbg = 1;
		DoubleBuffer[1].Draw.r0 = c >> 16;
		DoubleBuffer[1].Draw.g0 = c >> 8;
		DoubleBuffer[1].Draw.b0 = Db_SkyColor;
	}
}
