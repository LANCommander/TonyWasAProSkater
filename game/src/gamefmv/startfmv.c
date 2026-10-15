/* GameFMV_StartFMV__Fi (0x80023270) */
#include "names.h"

typedef struct {
	char *name;
	unsigned short width, height, endframe, pad0a;
	unsigned int vlcbuffersize, ringbuffersectors;
	int is24bit;
	int p;
	int pad2;
} FE;

extern FE movieDetails[];
extern unsigned int *decodeSliceBuffer[];

extern int GameFMV_Active, GameFMV_IsMovie, currentDecodeBuffer, currentVlcBuffer, sendVLC, alreadyHandledPause, GameFMV_RewindSwitch;
extern unsigned int *vlcBuffer[2];
extern int GameFMV_VLCBufferSize, GameFMV_RingBufferSectors;
extern unsigned int *pRingBuffer;
extern unsigned char GameFMV_Is24Bit, GameFMV_SliceBuffers;
extern unsigned short GameFMV_StartX, GameFMV_StartY, GameFMV_Width, GameFMV_Height;
extern unsigned short decodeSlice[4]; /* RECT */
extern unsigned char GameFMV_CurrentTrack;

void *Mem_NewX(unsigned int, int, int);
void DecDCTvlcBuild3(void);
void strInit(void);
int strNextVlc(int);

void GameFMV_StartFMV(int a) {
	int i, j;
	unsigned t, u;

	if (GameFMV_Active == 0 && movieDetails[GameFMV_CurrentTrack].p != 0) {
		decodeSlice[0] = GameFMV_StartX;
		decodeSlice[1] = GameFMV_StartY;
		decodeSlice[2] = GameFMV_Is24Bit ? 24 : 16;
		alreadyHandledPause = 0;
		GameFMV_IsMovie = 0;
		currentDecodeBuffer = 0;
		currentVlcBuffer = 0;
		sendVLC = 0;
		decodeSlice[3] = GameFMV_Height;

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
