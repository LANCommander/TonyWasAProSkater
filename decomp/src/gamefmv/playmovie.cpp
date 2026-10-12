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
#include "game/pad.h"

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
extern short DoubleBuffer_68[];
extern int MIDILEVEL;
extern u_long *decodeSliceBuffer[];
extern SControl Pad[];

int strNextVlc(int);
void Front_ClearScreen(void);
void handleLidOpen(void);
void Pad_ClearTriggers(SControl *);

extern "C" {
	void FUN_80023168(int); /* GameFMV_SetStartTrack */
	void *Mem_NewX__FUiii(int, int, int);
	void Mem_Delete__FPv(void *);
	void DecDCTvlcBuild3(void);
	DISPENV *FUN_800914fc(DISPENV *, int, int, int, int);
	void FUN_8008eebc(int);
	int FUN_8008ca0c(DISPENV *);
	void FUN_8008f620(CdlATV *);
	void strInit__Fv(void);
	void FUN_80070ff4(void);
	void FUN_800902c0(u_long *, int);
	void FUN_8009033c(u_long *, int);
	int FUN_8008f264(int, int, uchar *);
	void FUN_8009035c(int);
	void FUN_8009074c(void);
	void FUN_8008f4d4(int, int, int);
}

#ifdef PC_PORT
#include "pc_movie.h"
#endif

void GameFMV_PlayMovie(int track) {
	DISPENV disp;
	short old_screen_w;
	CdlATV atv;

	if (GameFMV_Active)
		return;

	GameFMV_IsMovie = 1;
	FUN_80023168(track);

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
	old_screen_w = DoubleBuffer_68[0];

	{
		int i;

		for (i = 0; i < 2; i++)
			if (!(vlcBuffer[i] = (u_long *)Mem_NewX__FUiii(GameFMV_VLCBufferSize, 1, 0)))
				goto InsufficientMemory;
	}

	pRingBuffer = (u_long *)Mem_NewX__FUiii(GameFMV_RingBufferSectors << 11, 1, 0);

	if (!pRingBuffer)
		goto InsufficientMemory;

	GameFMV_SliceBuffers = 2;

	{
		int i;

		for (i = 0; i < GameFMV_SliceBuffers; i++) {
			int h = GameFMV_Height;

			if (!(decodeSliceBuffer[i] = (u_long *)Mem_NewX__FUiii(GameFMV_Is24Bit ? h * 48 : h * 32, 1, 0)))
				goto InsufficientMemory;
		}
	}

	{
		DecDCTvlcBuild3();

		GameFMV_Active = 1;
		Front_ClearScreen();
		FUN_800914fc(&disp, 0, 256, 320, 240);
		FUN_8008eebc(0);
		FUN_8008ca0c(&disp);

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

		FUN_8008f620(&atv);
	}

	strInit__Fv();
	FUN_80070ff4();

	Pad[0].Pause.Triggered = 0;
	Pad[0].MenuAccept.Triggered = 0;
#ifdef PC_PORT
	Pad[0].MenuBack.Triggered = 0;
#endif

#ifdef PC_PORT
	/* a Dreamcast film: its audio streams as an STR's would, the pictures come from
	   pc_movie.c in 24-bit at (0, 256) */
	if (PC_MovieOpen()) {
		FUN_800914fc(&disp, 0, 256, 480, 240);
		disp.isrgb24 = 1;
		disp.disp.w = 320;
		FUN_8008eebc(0);
		FUN_8008ca0c(&disp);

		while (PC_MovieFrame()) {
			FUN_80070ff4();

			if (Pad[0].Pause.Triggered || Pad[0].MenuAccept.Triggered || Pad[0].MenuBack.Triggered)
				break;
		}

		PC_MovieClose();
		goto end;
	}
#endif

#ifdef PC_PORT
	/* the CD stream delivers frames on the vblank clock: wait for them */
	while (!strNextVlc(1))
		if (!PC_MovieWait())
			goto end;
#else
	while (!strNextVlc(1))
		;
#endif

	for (;;) {
		FUN_800902c0(vlcBuffer[currentVlcBuffer], GameFMV_Is24Bit ? 3 : 2);
		FUN_8009033c(decodeSliceBuffer[currentDecodeBuffer], decodeSlice_4 * decodeSlice_6 / 2);

		{
			uchar result[8];
			int count;

			FUN_8008f264(1, 0, result);

			if (result[0] & 16) {
				handleLidOpen();
				break;
			}

			count = 0x800000;

			while (!strNextVlc(1)) {
				if (!count)
					goto end;

				count--;
#ifdef PC_PORT
				if (!PC_MovieWait())
					goto end;
#endif
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

		FUN_800914fc(&disp, 0, (decodeSlice_2 < 256) << 8, GameFMV_Is24Bit ? 480 : 320, 240);

		if (GameFMV_Is24Bit) {
			disp.isrgb24 = 1;
			disp.disp.w = disp.disp.w * 2 / 3;
		}

		FUN_8008eebc(0);
		FUN_8008ca0c(&disp);

		if (GameFMV_RewindSwitch)
			break;

		FUN_80070ff4();

		if (Pad[0].Pause.Triggered)
			break;

		if (Pad[0].MenuAccept.Triggered)
			break;
#ifdef PC_PORT
		if (Pad[0].MenuBack.Triggered)
			break;
#endif
	}
end:
	Pad_ClearTriggers(Pad);

	GameFMV_Active = 0;
	Front_ClearScreen();

	disp.isrgb24 = 0;
	disp.disp.w = 512;
	disp.screen.w = old_screen_w;
	FUN_8008eebc(0);
	FUN_8008ca0c(&disp);

	atv.val0 = 0;
	atv.val1 = 0;
	atv.val2 = 0;
	atv.val3 = 0;
	FUN_8008f620(&atv);
	FUN_8009035c(0);
	FUN_8009074c();
	FUN_8008f4d4(9, 0, 0);
InsufficientMemory:
	if (pRingBuffer) {
		Mem_Delete__FPv(pRingBuffer);

		pRingBuffer = 0;
	}

	{
		int i;

		for (i = 0; i < 2; i++) {
			if (vlcBuffer[i]) {
				Mem_Delete__FPv(vlcBuffer[i]);

				vlcBuffer[i] = 0;
			}
		}
	}

	{
		int i;

		for (i = 0; i < GameFMV_SliceBuffers; i++) {
			if (decodeSliceBuffer[i]) {
				Mem_Delete__FPv(decodeSliceBuffer[i]);

				decodeSliceBuffer[i] = 0;
			}
		}
	}
}
