/* FUN_80082054 (0x80082054) */
#include "names.h"

extern short WibbleTables[];
extern short rcossin_tbl;
extern int M3d_FadeColour;

void M3dInit_SetResolution(unsigned int, unsigned int);
void M3dInit_SetFoggingParams(int, int, unsigned int);

void FUN_80082054(void) {
	int i, j;
	short *out;

	M3dInit_SetResolution(512, 240);
	M3dInit_SetFoggingParams(10, 6000, 2048);

	for (i = 0, out = WibbleTables; i < 16; i++)
		for (j = 0; j < 4096; j += 64)
			*out++ = ((&rcossin_tbl)[j * 2] * i) >> 4;

	M3d_FadeColour = 0x80000000;
}
