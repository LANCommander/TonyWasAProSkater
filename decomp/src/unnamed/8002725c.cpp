/* __10CGPolyLinei (0x8002725c) */
/* C++ (BIT2.CPP). A gouraud poly-line of n segments, all white and opaque at the origin; links itself
   into GPolyLineList. */
#include "game/bit.h"
#include "game/m3d.h"

extern CVector ZeroVector;
extern CBit *GPolyLineList;

void *Mem_NewX(uint, int, int);

CGPolyLine::CGPolyLine(int n) {
	mSegs = (SLineSeg *)Mem_NewX(n << 4, 0, 1);

	for (int i = 0; i < n; i++) {
		*(uint *)&mSegs[i].r = 0x50ffffff;
		mSegs[i].End = ZeroVector;
	}

	mNumSegs = n;
	AttachTo(&GPolyLineList);

	mStartR = 255;
	mStartG = 255;
	mStartB = 255;
	mCode = 0x50;
}
