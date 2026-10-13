/* Video_SFXPlay__Fisi (0x80068e8c) */
#include "names.h"

typedef struct {
	int state, f4, f8, fc;
	unsigned *p;
	unsigned buf;
	int n;
	int f1c;
	char pad[0xa0];
} SVideoInfo;

extern SVideoInfo GVInfo1[];
extern int GamePaused;

void QueueSoundCmd(int, int);

void Video_SFXPlay(int a, short b, int c) {
	if (GVInfo1[0].state == 1 && GamePaused == 0)
		QueueSoundCmd(1, a);
}
