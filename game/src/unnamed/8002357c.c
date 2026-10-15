/* GameFMV_SyncFMV__Fi (0x8002357c) */
#include "names.h"
#include "pc_stream.h"
#include "pc_vdec.h"

extern short decodeSlice, decodeSlice_2, GameFMV_StartX, GameFMV_EndX;
extern unsigned char GameFMV_Is24Bit;

typedef struct {
	int x, y, z;
} V;

int DAT_800d1650, GameFMV_NumSoundSources, GameFMV_Active, alreadyHandledPause,
    GameFMV_RewindSwitch, sendVLC, currentVlcBuffer, currentDecodeBuffer;
short decodeSlice_4, decodeSlice_6;
unsigned int *vlcBuffer[2];

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
				/* the whole frame into the song's texture, where strCallback put its slices */
				if (GameFMV_StartX != -1)
					PC_VdecToTexture(vlcBuffer[currentVlcBuffer], GameFMV_Is24Bit,
					                 decodeSlice, decodeSlice_2, GameFMV_EndX, decodeSlice_6);

				currentVlcBuffer = 1 - currentVlcBuffer;
			}

			sendVLC = strNextVlc(a);
		}

		if (GamePaused || GameFrozen || MusicStopped) {
			if (alreadyHandledPause == 0) {
				alreadyHandledPause = 1;
				PC_StreamPause();
			}
		} else if (alreadyHandledPause) {
			alreadyHandledPause = 0;
			PC_StreamResume();
		}
	}
}
