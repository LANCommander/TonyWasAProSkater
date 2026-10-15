/* Video_HighlightStart__FP6CBruce (0x80068f84) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"
#include "game/bruce.h"

extern SVideoInfo GVInfo1[2];
extern CBruce *MechList;

extern int HLStartCalled, HighlightStartFrame, HighlightStartY;

void Video_HighlightStart(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (!HLStartCalled) {
		int t = v->NumFrames, x = b->mPos.vy;

		HLStartCalled = 1;
		HighlightStartFrame = t;
		HighlightStartY = x;
	}
}
