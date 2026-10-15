/* Pad_Button__FP7SButtoni (0x80070c74) */
#include "names.h"

typedef struct {
	unsigned char down, trig;
	int held, released, count;
} SButton;

extern int Pad_IdleTime;

extern void ResetMainDemoTimer(void);

void Pad_Button(SButton *b, int pressed) {
	b->count++;

	if (!b->down && pressed) {
		b->trig = 1;
		b->count = 0;
	}

	if (pressed) {
		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		b->down = 1;
	} else
		b->down = 0;

	if (b->down) {
		b->released = 0;
		b->held++;
	} else {
		b->held = 0;
		b->released++;
	}
}
