/* Video_SyncObs__Fi (0x80069534) */
#include "names.h"

typedef struct CBody CBody;

typedef struct {
	char pad[0x1c];
	struct CBruce *other;
} CBruce;

extern CBruce *MechList;
extern CBody *BaddyList, *EnvironmentalObjectList;

int ReplaySoundLockout;

void Ob_ClearInputFlags(CBody *);
void GetDataChunk(CBruce *, int);
void Ob_QuickAI(CBody **);
void Ob_AI(CBody **);

void Video_SyncObs(int count) {
	CBruce *b = MechList;
	CBruce *o;
	int i;

	ReplaySoundLockout = 1;
	o = b->other;

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
