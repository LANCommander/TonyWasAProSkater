/* Video_BloodSplat__FP6CBruceii (0x80068ee8) */
#include "names.h"

typedef struct CBruce CBruce;

typedef struct {
	int state, f4, f8, fc;
	unsigned *p;
	unsigned buf;
	int n;
	int f1c;
	char pad[0xa0];
} SVideoInfo;

extern SVideoInfo GVInfo1[];
extern CBruce *MechList;

int Splat1, Splat2;

void Video_BloodSplat(CBruce *b, int a, int c) {
	if (GVInfo1[0].state == 1) {
		if (b == MechList)
			Splat1 = a;
		else
			Splat2 = a;
	}
}
