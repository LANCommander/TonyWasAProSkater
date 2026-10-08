/* Video_HandleStepping__Fv (0x8006ac84) */
#include "names.h"
typedef struct {
	unsigned char b[0x16c];
} SControl;

extern SControl Pad[];
extern unsigned char DAT_800c0a48b[];

extern struct {
	int mode;
	char _4[0xb8];
	int step;
} GVInfo1;

extern int DemoTimer, Status, GNumberOfPlayers, GameFrozen, MusicStopped;

int DAT_800d1df0;

void Video_HandleStepping(void) {
	int i, v;

	if (GVInfo1.mode != 2 || DemoTimer == 0) {
		if (DemoTimer != 0)
			return;

		for (i = 0; i < 2; i++) {
			if (DAT_800c0a48b[i * 364 + 1] || DAT_800c0a48b[i * 364 + 0x11] ||
			    DAT_800c0a48b[i * 364 + 0x41] || DAT_800c0a48b[i * 364 + 0x51] ||
			    DAT_800c0a48b[i * 364 + 0x61] || DAT_800c0a48b[i * 364 + 0x71] ||
			    DAT_800c0a48b[i * 364 + 0x81] || DAT_800c0a48b[i * 364 + 0x91] ||
			    DAT_800c0a48b[i * 364 + 0xa1] || DAT_800c0a48b[i * 364 + 0xb1] ||
			    DAT_800c0a48b[i * 364 + 0x21] || DAT_800c0a48b[i * 364 + 0xf1])
				DAT_800c0a48b[i * 364 + 0x31] = 1;
		}

		return;
	}

	if (Status == 20 || Status == 22 || Status == 21)
		return;

	if (Pad[0].b[0x11] || (GNumberOfPlayers == 2 && Pad[1].b[0x11])) {
		v = GVInfo1.step;
		Pad[0].b[0x11] = 0;
		Pad[1].b[0x11] = 0;
		v ^= 1;
		GVInfo1.step = v;
		GameFrozen = v;
		MusicStopped = v;
	}

	if (GVInfo1.step == 0)
		return;

	GameFrozen = 1;

	if (Pad[0].b[0x20] || (GNumberOfPlayers == 2 && Pad[1].b[0x20])) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 6;
		GameFrozen = 0;
		return;
	}

	if (Pad[0].b[0] || (GNumberOfPlayers == 2 && Pad[1].b[0])) {
		if (DAT_800d1df0 != 0) {
			DAT_800d1df0--;
			return;
		}

		DAT_800d1df0 = 1;
		GameFrozen = 0;
		return;
	}

	DAT_800d1df0 = 0;
	return;
}
