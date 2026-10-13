/* SFX_ProcessingPerFrame__Fv (0x8004bf30) */
#include "names.h"

int keyon_queue, keyoff_queue, reverb_queue;
unsigned char reverb_changed;
unsigned voicefree_flag, nko_flag, loop_flag, voiceon_flag, exp_flag;

void FUN_800947e4(int, int);
void FUN_80094c74(int, int);
void FUN_800947c4(int, short *);

void SFX_ProcessingPerFrame(void) {
	short st[4];
	int i;
	unsigned m;

	voicefree_flag = 0;

	if (keyon_queue) {
		FUN_800947e4(1, keyon_queue);

		keyon_queue = 0;
	}

	if (keyoff_queue) {
		FUN_800947e4(0, keyoff_queue);

		keyoff_queue = 0;
	}

	if (reverb_changed) {
		FUN_80094c74(8, reverb_queue);

		reverb_changed = 0;
	}

	for (i = 0; i < 24; i++) {
		m = 1 << i;

		if (nko_flag & m)
			continue;

		if (loop_flag & m)
			continue;

		FUN_800947c4(i, st);

		if (voiceon_flag & m) {
			if (st[0])
				continue;

			if (exp_flag & m)
				voiceon_flag &= ~m;
			else
				exp_flag |= m;
		} else {
			if (st[0] == 0)
				voicefree_flag |= m;
		}
	}

	nko_flag = 0;
}
