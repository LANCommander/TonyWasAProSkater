/* Video_SyncObsIfNeeded__Fv (0x800695f4) */
#include "names.h"

typedef struct {
	short a, b;
	int c;
} SVideoMark;

typedef struct {
	int state, f4, f8, fc;
	unsigned *p;
	unsigned buf;
	int n;
	int f1c;
	char pad[0x30];
	SVideoMark marks[14];
} SVideoInfo;

extern SVideoInfo GVInfo1[];

int GVideoReplay;

void Video_SyncObs(int);

void Video_SyncObsIfNeeded(void) {
	int n = GVideoReplay;

	if (n == 1) {
		if (GVInfo1[0].marks[0].a == 1)
			return;

		Video_SyncObs(GVInfo1[0].marks[0].a - 1);
	} else {
		Video_SyncObs(GVInfo1[0].marks[n - 1].a - GVInfo1[0].marks[n - 2].b - 2);
	}
}
