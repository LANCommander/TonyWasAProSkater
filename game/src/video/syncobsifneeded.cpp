/* Video_SyncObsIfNeeded__Fv (0x800695f4) */
/* C++ (VIDEO.CPP). */
#include "game/video.h"

extern SVideoInfo GVInfo1[2];

extern int GVideoReplay;

void Video_SyncObs(int);

void Video_SyncObsIfNeeded() {
	int n = GVideoReplay;

	if (n == 1) {
		if (GVInfo1[0].Highlights[0].FromFrame == 1)
			return;

		Video_SyncObs(GVInfo1[0].Highlights[0].FromFrame - 1);
	} else {
		Video_SyncObs(GVInfo1[0].Highlights[n - 1].FromFrame - GVInfo1[0].Highlights[n - 2].ToFrame - 2);
	}
}
