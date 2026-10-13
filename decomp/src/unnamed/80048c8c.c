/* Db_UpdateSky__Fv (0x80048c8c) */
#include "names.h"

typedef struct {
	char pad[24];
	unsigned char on, r, g, b;
	char rest[0x80ac - 28];
} T;

extern T DoubleBuffer[2];
extern T DAT_800b09c8;

int Db_SkyColor;

void Db_UpdateSky(void) {
	unsigned int c = Db_SkyColor;

	if (c == -1) {
		DoubleBuffer[0].on = 0;
		DoubleBuffer[1].on = 0;
	} else {
		DoubleBuffer[0].on = 1;
		DoubleBuffer[0].r = c >> 16;
		DoubleBuffer[0].g = c >> 8;
		DoubleBuffer[0].b = Db_SkyColor;
		DoubleBuffer[1].on = 1;
		DAT_800b09c8.r = c >> 16;
		DAT_800b09c8.g = c >> 8;
		DAT_800b09c8.b = Db_SkyColor;
	}
}
