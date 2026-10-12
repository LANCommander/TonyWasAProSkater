/* DisplayTrick__10CPanelDataP6CBruceUsUs (0x8006d938) */
/* C++ (PANEL.CPP). Demo twin CPanelData::DisplayTrick by signature and prologue (the demo body
   uses the Font/TextEffect classes THPS1 lacks). Draws the three trick-name lines bottom-up and
   animates the text scale: grow from 60 (state 1), settle to normal (2), hold for 30 frames (3),
   shrink away (4), or hand over to the message effect at +0x164 (5). Called on pBruce->mPanel.
   The per-mode scale pair is set in both arms (NormalScale first): that keeps jump.c from
   rewriting the if/else into "x = b; if (c) x = a;", giving retail's bne; j shape. */
#include "game/bruce.h"
#include "game/video.h"
#include "game/career.h"

extern SVideoInfo GVInfo1;
extern SCareer GCareer;
extern int GNumberOfPlayers;
extern int GameFrozen;

void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetScale(int);
int Mess_GetScale(void);
int Mess_DrawPanelText(int, int, const char *, int, uint, int);

extern "C" {
	void GraffitiColor__FP6CBruce(CBruce *);
	void Mess_SetTextJustify__FUc(int);
	void FUN_8001e0a4(FallingTextEffect *, int);
	void FUN_8001e0f8(FallingTextEffect *, ushort);
	ushort FUN_8001e108(FallingTextEffect *);
}

void CPanelData::DisplayTrick(CBruce *pBruce, ushort xpos, ushort ypos) {
	int ok = GVInfo1.VideoMode != 2;

	if (!ok)
		return;

	if (!GCareer.Options.ScoreDisplayEnabled)
		return;

	Mess_SetRGB(128, 128, 128, 0);
	GraffitiColor__FP6CBruce(pBruce);
	Mess_SetTextJustify__FUc(0);
	int NormalScale;
	int BigScale;

	if (GNumberOfPlayers == 1) {
		NormalScale = 188;
		BigScale = 256;
	} else {
		NormalScale = 188;
		BigScale = 188;
	}

	switch (mTrickState) {
		case 0:
			return;
		case 1: {
			int Frames = 5;

			Mess_SetScale((BigScale - 60) * mTrickTimer / Frames + 60);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 2;
			}

			break;
		}
		case 2: {
			int Frames = 5;

			Mess_SetScale(BigScale + (NormalScale - BigScale) * mTrickTimer / Frames);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 3;
			}

			break;
		}
		case 3:
			Mess_SetScale(188);

			if (mTrickTimer == 30) {
				mTrickTimer = 0;
				mTrickState = 4;
			}

			if (mLastMult) {
				mTrickState = 5;
				mLastMult = 0;
			}

			break;
		case 4: {
			int Frames = 10;

			Mess_SetScale(NormalScale + (0 - NormalScale) * mTrickTimer / Frames);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 0;
			}

			if (mLastMult)
				mLastMult = 0;

			break;
		}
		case 5:
			break;
	}

	if (mTrickState == 5)
		FUN_8001e0a4(&pBruce->mPanel.mFallingTextEffect, 1);
	else
		FUN_8001e0f8(&pBruce->mPanel.mFallingTextEffect, Mess_GetScale());

	if (!GameFrozen)
		mTrickTimer++;

	int LineHeight = Mess_GetScale() * 13 / 188;

	if (pBruce->mPanel.mTrickName3[0]) {
		Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName3, 0, 0, 256);

		ypos -= LineHeight;
	}

	if (pBruce->mPanel.mTrickName2[0]) {
		Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName2, 0, 0, 256);

		ypos -= LineHeight;
	}

	Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName, 0, 0, 256);
	int Busy = FUN_8001e108(&pBruce->mPanel.mFallingTextEffect);

	if (mTrickState == 5) {
		FUN_8001e0a4(&pBruce->mPanel.mFallingTextEffect, 0);

		if (!Busy)
			pBruce->mPanel.mTrickState = 0;
	}
}
