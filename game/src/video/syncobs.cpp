/* Video_SyncObs__Fi (0x80069534) */
/* C++ (VIDEO.CPP). */
#include "game/bruce.h"
#include "game/video.h"

extern CBruce *MechList;
extern CBody *BaddyList, *EnvironmentalObjectList;

int ReplaySoundLockout;

void Video_SyncObs(int count) {
	CBruce *b = MechList;
	CBruce *o;
	int i;

	ReplaySoundLockout = 1;
	o = (CBruce *)b->mNextItem;

	for (i = 0; i < count; i++) {
		Ob_ClearInputFlags(BaddyList);
		Ob_ClearInputFlags(EnvironmentalObjectList);
		GetDataChunk(b, -1);

		if (o)
			GetDataChunk(o, -1);

		Ob_QuickAI(&EnvironmentalObjectList);
		Ob_AI(&BaddyList);
	}

	ReplaySoundLockout = 0;
}
