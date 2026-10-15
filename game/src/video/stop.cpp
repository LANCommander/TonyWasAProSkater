/* Video_Stop__FP6CBruce (0x8006981c) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"

class CBruce;

extern SVideoInfo GVInfo1[2];
extern CBruce *MechList;

int GVideoReplay;

void FlushBits(SVideoInfo *);

void Video_Stop(CBruce *b) {
	SVideoInfo *v;

	if (b == MechList)
		v = &GVInfo1[0];
	else
		v = &GVInfo1[1];

	if (v->VideoMode == VIDEO_RECORD)
		FlushBits(v);

	v->VideoMode = VIDEO_OFF;
	GVideoReplay = 0;
}
