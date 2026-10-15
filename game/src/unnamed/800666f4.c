/* LevelSelect_StartPicLoad__Fi (0x800666f4) */
#include "names.h"

int DAT_800d2374;
int DAT_800d2380;
int DAT_800d2388;
int DAT_800d2378;
int DAT_800d238c;
int DAT_800d2390;
int DAT_800d237c;
int DAT_800d2384;

extern void *DAT_800bf2f0[]; /* SlicedImage *[10] */
extern void *DAT_800d3810[];
extern int DAT_800d3840[];

void LevelSelect_StartPicLoad(int a) {
	int i;
	int n = DAT_800d237c;

	DAT_800d2374 = -1;
	DAT_800d2380 = 0;
	DAT_800d2388 = 0;
	DAT_800d2378 = a;
	DAT_800d238c = 1;
	DAT_800d2390 = 0;

	for (i = 0; i < n; i++) {
		DAT_800d3840[i] = 0;
		DAT_800d3810[i] = 0;
		DAT_800bf2f0[i] = 0;
	}

	DAT_800d2384 = 0;
}
