/* Video_HighlightEnd__FP6CBrucei (0x80068fd8) flags=-fstructure-noalias */
/* C++ (VIDEO.CPP). Demo twin. Inserts a finished trick into the player's top-5 replay highlights. */
#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

#include "game/video.h"
#include "game/bruce.h"

extern SVideoInfo GVInfo1[2];
extern CBruce *MechList;

int HLStartCalled;
int HighlightStartFrame;
int HighlightEndFrame;
int HighlightStartY;
int HighlightEndY;

void Video_HighlightEnd(CBruce *pSkater, int Score) {
	SVideoInfo *VInfo;
	int WasABail;

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	HLStartCalled = 0;
	HighlightEndFrame = VInfo->NumFrames;
	HighlightEndY = pSkater->mPos.vy;
	WasABail = (Score < 0);

	if (Score < 0)
		Score = -Score;

	if (Score == 0)
		Score = (ABS(HighlightEndY - HighlightStartY) * 2) >> 12;
	else
		Score += ABS(HighlightEndY - HighlightStartY) >> 13;

	for (int i = 0; i < 5; i++) {
		if (VInfo->Highlights[i].Score < Score) {
			for (int j = 4; j > i; j--)
				VInfo->Highlights[j] = VInfo->Highlights[j - 1];

			HighlightStartFrame -= 15;

			if (HighlightStartFrame < 0)
				HighlightStartFrame = 0;

			if (WasABail)
				HighlightEndFrame += 30;
			else
				HighlightEndFrame += 8;

			if (HighlightEndFrame >= 3600)
				HighlightEndFrame = 3599;

			VInfo->Highlights[i].FromFrame = HighlightStartFrame;
			VInfo->Highlights[i].ToFrame = HighlightEndFrame;
			VInfo->Highlights[i].Score = Score;
			break;
		}
	}
}
