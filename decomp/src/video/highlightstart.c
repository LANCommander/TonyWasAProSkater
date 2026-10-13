/* Video_HighlightStart__FP6CBruce (0x80068f84) */
#include "names.h"

typedef struct {
	int f0, f4, f8;
} CBruce;

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

int HLStartCalled, HighlightStartFrame, HighlightStartY;

void Video_HighlightStart(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (!HLStartCalled) {
		int t = v->f1c, x = b->f8;

		HLStartCalled = 1;
		HighlightStartFrame = t;
		HighlightStartY = x;
	}
}
