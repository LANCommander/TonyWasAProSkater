/* GameFMV_SyncFMV__Fi (0x8002357c) */
#include "names.h"
#ifdef PC_PORT
#include "pc_stream.h"
#endif

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
extern void GameFMV_SetCDMixForPos(void *);
extern void DecDCTvlcBuild3(void);
extern void DecDCTin(int, int);
extern void DecDCTout(int, int);
extern void StSetMask(int, int, int);
extern void CdControlB(int, int, int);
extern void CdRead2(int);
extern void strKickCD(void);
extern int strNextVlc(int);

void GameFMV_SyncFMV(int a) {
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

			GameFMV_SetCDMixForPos((char *)DAT_800a6388 + bi * 12);

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
				DecDCTin(vlcBuffer[currentVlcBuffer], 2);

				w = decodeSlice_4 * decodeSlice_6;
				currentVlcBuffer = 1 - currentVlcBuffer;
				DecDCTout(decodeSliceBuffer[currentDecodeBuffer], w / 2);
			}

			sendVLC = strNextVlc(a);
		}

		if (GamePaused || GameFrozen || MusicStopped) {
			if (alreadyHandledPause == 0) {
				alreadyHandledPause = 1;
#ifdef PC_PORT
				PC_StreamPause();
#else
				StSetMask(1, 0, -1);
				CdControlB(9, 0, 0);
#endif
			}
		} else if (alreadyHandledPause) {
			alreadyHandledPause = 0;
#ifdef PC_PORT
			PC_StreamResume();
#else
			StSetMask(0, 0, -1);
			CdRead2(480);
#endif
		}
	}
}
