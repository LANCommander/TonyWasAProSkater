/* __15CTexturedRibbonii (0x8002e1e0) */
/* C++ (BIT.CPP). A textured ribbon of NumPoints points, all zero width. */
#include "game/bit.h"

void *Mem_NewX(unsigned int, int, int);

CTexturedRibbon::CTexturedRibbon(int NumPoints, int Trail) {
	mNumPoints = NumPoints;
	mpPoints = (SRibbonPoint *)Mem_NewX(NumPoints * 36, 0, 1);

	for (int i = 0; i < mNumPoints; i++) {
		mpPoints[i].WidthB = 0;
		mpPoints[i].Width = 0;
	}

	mTrail = Trail;
	mDv = 8;
}
