/* Video_BloodSplat__FP6CBruceii (0x80068ee8) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"

class CBruce;

extern SVideoInfo GVInfo1[2];
extern CBruce *MechList;

int Splat1, Splat2;

void Video_BloodSplat(CBruce *b, int a, int c) {
	if (GVInfo1[0].VideoMode == VIDEO_RECORD) {
		if (b == MechList)
			Splat1 = a;
		else
			Splat2 = a;
	}
}
