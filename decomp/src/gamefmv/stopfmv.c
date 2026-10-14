/* GameFMV_StopFMV__Fv (0x800233d0) */
#include "names.h"
#ifdef PC_PORT
#include "pc_stream.h"
#include "pc_texstore.h"
#endif

typedef struct {
	unsigned short x, y, w, h;
} RECT;

extern int decodeSliceBuffer[];

int GameFMV_Active, GameFMV_FrameBufferCopyActive, GameFMV_NumSoundSources, GameFMV_StartSector,
    pRingBuffer;
int vlcBuffer[2];
unsigned char GameFMV_SliceBuffers;
short GameFMV_StartX;
unsigned short GameFMV_StartY, GameFMV_Width, GameFMV_Height;

void CdMix(unsigned char *);
void DecDCToutCallback(int);
void StUnSetRing(void);
void FUN_800912bc(int, unsigned char *);
void CdControl(int, unsigned char *, int);
void Mem_Delete(int);
void ClearImage(RECT *, int, int, int);

void GameFMV_StopFMV(void) {
	unsigned char p[4];
	unsigned char loc[4];
	RECT r;
	int i;

	GameFMV_NumSoundSources = 0;
	GameFMV_FrameBufferCopyActive = 0;

	if (GameFMV_Active) {
		p[0] = 0;
		p[1] = 0;
		p[2] = 0;
		p[3] = 0;
#ifdef PC_PORT
		PC_StreamMix(p);
#else
		CdMix(p);
#endif

		GameFMV_Active = 0;
#ifdef PC_PORT
		PC_StreamStop();
#else
		DecDCToutCallback(0);
		StUnSetRing();
		FUN_800912bc(GameFMV_StartSector, loc);
		CdControl(22, loc, 0);
#endif

		if (pRingBuffer) {
			Mem_Delete(pRingBuffer);

			pRingBuffer = 0;
		}

		for (i = 0; i < GameFMV_SliceBuffers; i++) {
			if (decodeSliceBuffer[i]) {
				Mem_Delete(decodeSliceBuffer[i]);

				decodeSliceBuffer[i] = 0;
			}
		}

		for (i = 0; i < 2; i++) {
			if (vlcBuffer[i]) {
				Mem_Delete(vlcBuffer[i]);

				vlcBuffer[i] = 0;
			}
		}

		if (GameFMV_StartX != -1) {
			r.x = GameFMV_StartX;
			r.y = GameFMV_StartY;
			r.w = GameFMV_Width;
			r.h = GameFMV_Height;
#ifdef PC_PORT
			PC_TexFill(r.x, r.y, r.w, r.h, 0x010101);
#else
			ClearImage(&r, 1, 1, 1);
#endif
		}
	}
}
