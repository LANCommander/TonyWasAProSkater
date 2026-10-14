/* GameFMV_StartFMV__Fi (0x80023270) */
#include "names.h"

typedef struct {
	char pad[0x18];
	int p;
	int pad2;
} FE;

extern FE movieDetails[];
extern int decodeSliceBuffer[];

int GameFMV_Active, GameFMV_IsMovie, currentDecodeBuffer, currentVlcBuffer, sendVLC,
    alreadyHandledPause, GameFMV_RewindSwitch;
int vlcBuffer[2];
int GameFMV_VLCBufferSize, GameFMV_RingBufferSectors, pRingBuffer;
unsigned char GameFMV_Is24Bit, GameFMV_SliceBuffers;
unsigned short GameFMV_StartX, GameFMV_StartY, GameFMV_Width, GameFMV_Height, decodeSlice,
    decodeSlice_2, decodeSlice_4, decodeSlice_6;
unsigned char GameFMV_CurrentTrack;

int Mem_NewX(int, int, int);
void DecDCTvlcBuild3(void);
void strInit(void);
int strNextVlc(int);

void GameFMV_StartFMV(int a) {
	int i, j;
	unsigned t, u;

	if (GameFMV_Active == 0 && movieDetails[GameFMV_CurrentTrack].p != 0) {
		decodeSlice = GameFMV_StartX;
		decodeSlice_2 = GameFMV_StartY;
		decodeSlice_4 = GameFMV_Is24Bit ? 24 : 16;
		alreadyHandledPause = 0;
		GameFMV_IsMovie = 0;
		currentDecodeBuffer = 0;
		currentVlcBuffer = 0;
		sendVLC = 0;
		decodeSlice_6 = GameFMV_Height;

		if (a) {
			for (i = 0; i < 2; i++)
				vlcBuffer[i] = Mem_NewX(GameFMV_VLCBufferSize, 1, 1);
		}

		pRingBuffer = Mem_NewX(GameFMV_RingBufferSectors << 11, 1, 1);
		t = GameFMV_Width;
		t >>= 4;
		GameFMV_SliceBuffers = t;

		/* PC: frames are decoded whole (pc_vdec.h), no slices */
		GameFMV_SliceBuffers = 0;

		if (a) {
			u = t & 255;

			if (u) {
				j = 0;

				do {
					decodeSliceBuffer[j] = Mem_NewX(GameFMV_Height << 5, 1, 1);
					j++;
				} while (j < GameFMV_SliceBuffers);
			}

			DecDCTvlcBuild3();
		}

		GameFMV_Active = 1;
		GameFMV_RewindSwitch = 0;
		strInit();
		strNextVlc(1);
	}
}
