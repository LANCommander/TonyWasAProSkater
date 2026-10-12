/* __5CGlowUiUi (0x80028c48) */
/* C++ (BIT.CPP). Fuzzy demo twin; members and locals from its MAIN.SYM. Written as a real constructor so
   `this` is copied where retail copies it. */
#include "game/bit.h"

void *Mem_New(uint, int, int);

char GlowList[8];

CGlow::CGlow(uint NumPoints, uint NumFringes) {
	mNumSections = NumPoints * 2;
	mNumFringes = NumFringes;
	mStepAngle = 4096 / mNumSections;
	mpSections =
	    (SSection *)Mem_New((mNumSections + mNumSections * mNumFringes) * sizeof(SSection), 0, 1);
	mpFringes = (SFringeQuad *)(mpSections + mNumSections);

	for (uint i = 0; i < mNumSections * mNumFringes; i++)
		mpFringes[i].CodeBGR= 0x3a000000;

	mCentreCodeBGR = 0x32000000;
	mMask = 0xffffffff;
	AttachTo(GlowList);
}
