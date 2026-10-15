/* FUN_8006dc78 (0x8006dc78) */
#include "game/bruce.h"
#include "game/panel.h"
#include "game/mess.h"

extern int Cheat_PanelOn;
extern int GamePaused;
extern int ViewportMode;
extern CBruce *MechList[2];

int TAPES_Display, SKATE_Display;

void DisplayTapesSkate();
extern "C" void FUN_8001e100(FallingTextEffect *, int);
void DisplayScore(CBruce *, ushort, ushort);
extern "C" void FUN_8006d0c8(CBruce *, int, int);
void DisplayTime(CBruce *, ushort, ushort);
void DisplayOutOfBounds(CBruce *, ushort, ushort);
void Panel_ReplayInstructions();
void TrickTips_Display();

extern "C" void FUN_8006dc78() {
	CBruce *s2;
	FallingTextEffect *s3;

	if (Cheat_PanelOn) {
		s2 = MechList[0];
		Panel_ReplayInstructions();

		if (s2) {
			s3 = &s2->mPanel.mFallingTextEffect;

			do {
				s3->update();
				DisplayOutOfBounds(s2, 256, 90);
				DisplayTapesSkate();

				if (GamePaused) {
					SKATE_Display = 30;
					TAPES_Display = 30;
					return;
				}

				switch (ViewportMode * 2 + s2->mPanel.mPlayerNum) {
					case 0:
					case 1:
						TrickTips_Display();
						FUN_8006d0c8(s2, 256, 224);
						s2->mPanel.DisplayTrick(s2, 256, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 2:
						FUN_8006d0c8(s2, 384, 224);
						s2->mPanel.DisplayTrick(s2, 384, 205);
						DisplayScore(s2, 358, 45);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 4:
						s2->mPanel.DisplayTrick(s2, 256, 205);
						FUN_8006d0c8(s2, 256, 224);
						DisplayScore(s2, 25, 145);
						DisplayTime(s2, 452, 150);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 6:
						FUN_8006d0c8(s2, 128, 224);
						s2->mPanel.DisplayTrick(s2, 128, 205);
						DisplayScore(s2, 112, 150);
						DisplayTime(s2, 55, 155);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						Panel_DrawFlatShadedPoly(256, 0, 256, 120, 30, 30, 30, 1);
						break;
					case 8:
						FUN_8006d0c8(s2, 256, 224);
						s2->mPanel.DisplayTrick(s2, 256, 205);
						DisplayScore(s2, 70, 175);
						DisplayTime(s2, 133, 217);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 3:
						FUN_8006d0c8(s2, 128, 224);
						s2->mPanel.DisplayTrick(s2, 128, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 5:
						s2->mPanel.DisplayTrick(s2, 256, 85);
						FUN_8006d0c8(s2, 256, 104);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 452, 50);
						FUN_8001e100(s3, 118);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 118);
						break;
					case 7:
						FUN_8006d0c8(s2, 384, 104);
						s2->mPanel.DisplayTrick(s2, 384, 85);
						DisplayScore(s2, 276, 45);
						DisplayTime(s2, 457, 50);
						Panel_DrawFlatShadedPoly(0, 120, 257, 2, 30, 30, 30, 1);
						Panel_DrawFlatShadedPoly(0, 120, 257, 120, 20, 20, 20, 1);
						Panel_DrawFlatShadedPoly(256, 119, 512, 2, 30, 30, 30, 1);
						FUN_8001e100(s3, 122);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 122);
						break;
					case 9:
						FUN_8006d0c8(s2, 256, 154);
						s2->mPanel.DisplayTrick(s2, 256, 130);
						DisplayScore(s2, 315, 43);
						DisplayTime(s2, 371, 85);
						FUN_8001e100(s3, 152);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 152);
						break;
				}

				s2 = (CBruce *)s2->mNextItem;
				s3 = &s2->mPanel.mFallingTextEffect;
			} while (s2);
		}
	}
}
