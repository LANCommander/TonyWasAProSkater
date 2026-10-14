/* Video_Stop__FP6CBruce (0x8006981c) */
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

int GVideoReplay;

void FlushBits(SVideoInfo *);

void Video_Stop(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (v->state == 1)
		FlushBits(v);

	v->state = 0;
	GVideoReplay = 0;
}
