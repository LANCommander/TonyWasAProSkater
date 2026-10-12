/* Video_SortAndMerge__Fv (0x80069164) */
#include "game/video.h"

extern SVideoInfo GVInfo1[2];

void Video_SortAndMerge(void) {
	SHighlight TempHL[5];
	SHighlight NullHL;

	NullHL.FromFrame = NullHL.ToFrame = NullHL.Score = 0;

	{
		int i;

		for (i = 0; i < 5; i++) {
			TempHL[i] = NullHL;

			{
				int BestFrom = 0x7fffffff;
				int BestIndex = -1;
				int j;

				for (j = 0; j < 5; j++) {
					if (GVInfo1[0].Highlights[j].ToFrame >= GVInfo1[0].NumFrames)
						GVInfo1[0].Highlights[j].ToFrame = GVInfo1[0].NumFrames - 1;

					if (GVInfo1[0].Highlights[j].Score &&
					    GVInfo1[0].Highlights[j].FromFrame < BestFrom) {
						BestFrom = GVInfo1[0].Highlights[j].FromFrame;
						BestIndex = j;
					}
				}

				if (BestIndex != -1) {
					TempHL[i] = GVInfo1[0].Highlights[BestIndex];
					GVInfo1[0].Highlights[BestIndex].Score = 0;
				}
			}
		}
	}

	{
		int i;

		for (i = 4; i > 0; i--) {
			if (TempHL[i].FromFrame <= TempHL[i - 1].ToFrame + 15 &&
			    TempHL[i].FromFrame > TempHL[i - 1].FromFrame) {
				if (TempHL[i - 1].ToFrame < TempHL[i].ToFrame)
					TempHL[i - 1].ToFrame = TempHL[i].ToFrame;

				TempHL[i - 1].Score += TempHL[i].Score;
				TempHL[i] = NullHL;
			}
		}
	}

	{
		int i;
		int n = 0;

		for (i = 0; i < 5; i++) {
			GVInfo1[0].Highlights[i] = NullHL;

			if (TempHL[i].Score) {
				GVInfo1[0].Highlights[n] = TempHL[i];
				n++;
			}
		}
	}
}
