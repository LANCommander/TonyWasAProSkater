/* GameFMV_PlayMovie__Fi (0x80022c1c) */
/* C++ (GAMEFMV.CPP). Fuzzy demo twin (0.98; retail drops the demo's ExitDemo path), locals from its
   MAIN.SYM. Plays the current FMV: allocates the bitstream ring and slice buffers, switches the
   display to the movie's size (24-bit for hi-colour movies), sets the CD volume, then decodes
   frame by frame until the stream ends, a button is pressed or the lid opens. Then it restores
   the display and frees everything. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/dbuffer.h"
#include "game/gamefmv.h"
#include "game/mem.h"
#include "game/pad.h"
#include "game/psyq.h"
#include "pc_frame.h"
#include "pc_stream.h"

struct CdlATV {
	uchar val0, val1, val2, val3;
};

struct SMovie {
	char pad0[24];
	char *pName; /* 24 */
	char pad1c[32 - 28];
};

int GameFMV_IsMovie;
int GameFMV_RewindSwitch;
int GameFMV_Active;
int currentDecodeBuffer;
int currentVlcBuffer;
int sendVLC;
short GameFMV_StartX;
int GameFMV_VLCBufferSize;
int GameFMV_RingBufferSectors;
uchar GameFMV_SliceBuffers;
short GameFMV_StartY;
short GameFMV_EndX;
ushort GameFMV_Width;
ushort GameFMV_Height;
uchar GameFMV_Is24Bit;
uchar GameFMV_CurrentTrack;
short decodeSlice;
short decodeSlice_2;
short decodeSlice_4;
short decodeSlice_6;
u_long *vlcBuffer[2];
u_long *pRingBuffer;

extern SMovie movieDetails[];
extern int MIDILEVEL;
extern u_long *decodeSliceBuffer[];
extern SControl Pad[];

int strNextVlc(int);
void Front_ClearScreen(void);
void handleLidOpen(void);
void Pad_ClearTriggers(SControl *);

extern "C" {
	void DecDCTvlcBuild3(void);
}

#include "pc_movie.h"
#include "pc_screen.h"

void GameFMV_PlayMovie(int track) {
	DISPENV disp;
	short old_screen_w;
	CdlATV atv;

	if (!PC_MoviePlays(track))
		return;

	if (GameFMV_Active)
		return;

	GameFMV_IsMovie = 1;
	GameFMV_SetStartTrack(track);

	GameFMV_StartX = (320 - GameFMV_Width) / 2;
	GameFMV_EndX = GameFMV_StartX + (GameFMV_Is24Bit ? (GameFMV_Width * 3) / 2 : GameFMV_Width);
	GameFMV_StartY = (240 - GameFMV_Height) / 2;
	GameFMV_RewindSwitch = 0;

	if (!movieDetails[GameFMV_CurrentTrack].pName)
		return;

	decodeSlice = GameFMV_StartX;
	decodeSlice_2 = GameFMV_StartY;
	decodeSlice_6 = GameFMV_Height;
	decodeSlice_4 = GameFMV_Is24Bit ? 24 : 16;
	currentDecodeBuffer = 0;
	currentVlcBuffer = 0;
	sendVLC = 0;
	old_screen_w = DoubleBuffer[0].Disp.screen.w;

	{
		int i;

		for (i = 0; i < 2; i++)
			if (!(vlcBuffer[i] = (u_long *)Mem_NewX(GameFMV_VLCBufferSize, 1, 0)))
				goto InsufficientMemory;
	}

	pRingBuffer = (u_long *)Mem_NewX(GameFMV_RingBufferSectors << 11, 1, 0);

	if (!pRingBuffer)
		goto InsufficientMemory;

	/* PC: frames are decoded whole (PC_MovieDecode), no slices */
	GameFMV_SliceBuffers = 0;

	{
		int i;

		for (i = 0; i < GameFMV_SliceBuffers; i++) {
			int h = GameFMV_Height;

			if (!(decodeSliceBuffer[i] =
			          (u_long *)Mem_NewX(GameFMV_Is24Bit ? h * 48 : h * 32, 1, 0)))
				goto InsufficientMemory;
		}
	}

	{
		DecDCTvlcBuild3();

		GameFMV_Active = 1;
		Front_ClearScreen();
		SetDefDispEnv(&disp, 0, 256, 320, 240);
		PC_FrameWait(1);
		PC_ScreenShow(&disp);

		atv.val2 = 0;
		atv.val3 = 0;

		if (track == 3) {
			atv.val0 = 192;
			atv.val1 = 192;
		} else {
			int fmv_level;

			if (MIDILEVEL < 64)
				fmv_level = MIDILEVEL * 3;
			else
				fmv_level = (MIDILEVEL - 64) * 63 / 64 + 192;

			atv.val0 = fmv_level;
			atv.val1 = fmv_level;
		}

		PC_StreamMix(&atv.val0);
	}

	strInit();
	Pad_Update();

	Pad[0].Pause.Triggered = 0;
	Pad[0].MenuAccept.Triggered = 0;
	Pad[0].MenuBack.Triggered = 0;

	/* a Dreamcast film: its audio streams as an STR's would, the pictures come from
	   pc_movie.c */
	if (PC_MovieOpen()) {
		SetDefDispEnv(&disp, 0, 256, 480, 240);

		disp.isrgb24 = 1;
		disp.disp.w = 320;
		PC_FrameWait(1);

		while (PC_MovieFrame()) {
			Pad_Update();

			if (Pad[0].Pause.Triggered || Pad[0].MenuAccept.Triggered || Pad[0].MenuBack.Triggered)
				break;
		}

		PC_MovieClose();
		goto end;
	}

	/* the CD stream delivers frames on the vblank clock: wait for them */
	while (!strNextVlc(1))
		if (!PC_MovieWait())
			goto end;

	PC_MovieBegin(GameFMV_Is24Bit);

	for (;;) {
		/* the whole frame into the movie's screen (pc_movie.h), where strCallback put its slices */
		PC_MovieDecode(vlcBuffer[currentVlcBuffer], decodeSlice, decodeSlice_2 & 255, GameFMV_EndX,
		               decodeSlice_6);

		if (decodeSlice_2 < 256)
			decodeSlice_2 += 256;
		else
			decodeSlice_2 -= 256;

		{
			uchar result[8];
			int count;

			count = 0x800000;

			while (!strNextVlc(1)) {
				if (!count)
					goto end;

				count--;
				if (!PC_MovieWait())
					goto end;
			}

			if (!count)
				break;

			count = 0x800000;

			while (decodeSlice != GameFMV_StartX) {
				if (!count)
					goto end;

				count--;
			}

			if (!count)
				break;
		}

		SetDefDispEnv(&disp, 0, (decodeSlice_2 < 256) << 8, GameFMV_Is24Bit ? 480 : 320, 240);

		if (GameFMV_Is24Bit) {
			disp.isrgb24 = 1;
			disp.disp.w = disp.disp.w * 2 / 3;
		}

		PC_FrameWait(1);
		PC_MovieShow();

		if (GameFMV_RewindSwitch)
			break;

		Pad_Update();

		if (Pad[0].Pause.Triggered)
			break;

		if (Pad[0].MenuAccept.Triggered)
			break;
		if (Pad[0].MenuBack.Triggered)
			break;
	}
end:
	Pad_ClearTriggers(Pad);

	GameFMV_Active = 0;
	Front_ClearScreen();

	disp.isrgb24 = 0;
	disp.disp.w = 512;
	disp.screen.w = old_screen_w;
	PC_FrameWait(1);
	PC_MovieEnd();
	PC_ScreenShow(&disp);

	atv.val0 = 0;
	atv.val1 = 0;
	atv.val2 = 0;
	atv.val3 = 0;
	PC_StreamMix(&atv.val0);
	PC_StreamStop();
InsufficientMemory:
	if (pRingBuffer) {
		Mem_Delete(pRingBuffer);

		pRingBuffer = 0;
	}

	{
		int i;

		for (i = 0; i < 2; i++) {
			if (vlcBuffer[i]) {
				Mem_Delete(vlcBuffer[i]);

				vlcBuffer[i] = 0;
			}
		}
	}

	{
		int i;

		for (i = 0; i < GameFMV_SliceBuffers; i++) {
			if (decodeSliceBuffer[i]) {
				Mem_Delete(decodeSliceBuffer[i]);

				decodeSliceBuffer[i] = 0;
			}
		}
	}
}
