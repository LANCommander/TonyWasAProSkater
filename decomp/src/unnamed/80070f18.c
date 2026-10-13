/* FUN_80070f18 (0x80070f18) */
#include "names.h"

/* Maps a raw analog axis reading to -127..127 given its calibrated min/max and centre, with a
   dead zone of 32 either side of the centre. Movement outside it marks the pad active. */
int Pad_IdleTime;

void ResetMainDemoTimer(void);

int FUN_80070f18(int v, int min, int max, int centre) {
	if (v < min)
		return -127;

	if (max < v)
		return 127;

	if (v < centre - 32) {
		int a, n, b;

		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		a = v + 32;
		n = (centre - a) * -127;
		b = min + 32;
		return n / (centre - b);
	}

	if (centre + 32 < v) {
		int a, n, b;

		Pad_IdleTime = 0;
		ResetMainDemoTimer();

		a = v - 32;
		n = (a - centre) * 127;
		b = max - 32;
		return n / (b - centre);
	}

	return 0;
}
