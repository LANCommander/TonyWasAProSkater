/* FUN_8002357c (0x8002357c) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

int DAT_800d1650, GameFMV_NumSoundSources, GameFMV_Active, alreadyHandledPause,
    GameFMV_RewindSwitch, sendVLC, currentVlcBuffer, currentDecodeBuffer;
short decodeSlice_4, decodeSlice_6;
int vlcBuffer[2];

extern char *CameraList;
extern int GamePaused, GameFrozen, MusicStopped;
extern V DAT_800a6388[];
extern int decodeSliceBuffer[];

extern int Utils_CrapDist(void *, void *);
extern void FUN_80024010(void *);
extern void DecDCTvlcBuild3(void);
extern void FUN_800902c0(int, int);
extern void FUN_8009033c(int, int);
extern void FUN_8008f19c(int, int, int);
extern void FUN_8008f4d4(int, int, int);
extern void FUN_8008f0ec(int);
extern void strKickCD(void);
extern int strNextVlc(int);

void FUN_8002357c(int a) {
	int i, best, bi, d;
	V *p;

	if (DAT_800d1650) {
		DAT_800d1650 = DAT_800d1650 - 1;

		if (DAT_800d1650 == 0) {
			bi = 0;
			best = 0xffffff;

			for (i = 0; i < GameFMV_NumSoundSources; i++) {
				d = Utils_CrapDist(CameraList + 4, &DAT_800a6388[i]);

				if (d < best) {
					best = d;
					bi = i;
				}
			}

			FUN_80024010((char *)DAT_800a6388 + bi * 12);

			DAT_800d1650 = 4;
		}
	}

	if (GameFMV_Active) {
		int w;

		DecDCTvlcBuild3();

		if (alreadyHandledPause == 0) {
			if (GameFMV_RewindSwitch) {
				GameFMV_RewindSwitch = 0;
				strKickCD();
			}

			if (sendVLC) {
				FUN_800902c0(vlcBuffer[currentVlcBuffer], 2);

				w = decodeSlice_4 * decodeSlice_6;
				currentVlcBuffer = 1 - currentVlcBuffer;
				FUN_8009033c(decodeSliceBuffer[currentDecodeBuffer], w / 2);
			}

			sendVLC = strNextVlc(a);
		}

		if (GamePaused || GameFrozen || MusicStopped) {
			if (alreadyHandledPause == 0) {
				alreadyHandledPause = 1;
				FUN_8008f19c(1, 0, -1);
				FUN_8008f4d4(9, 0, 0);
			}
		} else if (alreadyHandledPause) {
			alreadyHandledPause = 0;
			FUN_8008f19c(0, 0, -1);
			FUN_8008f0ec(480);
		}
	}
}
