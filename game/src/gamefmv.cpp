/* GAMEFMV.CPP: the STR movies and songs - the movie table, full-screen playback, the in-level
   video screen (its texture, sound sources and CD mix) and the stream callbacks (declarations in
   game/gamefmv.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/gamefmv.h"
#include "game/m3d.h"
#include "pc_texstore.h"
#include "pc_stream.h"
#include "game/dbuffer.h"
#include "game/mem.h"
#include "game/pad.h"
#include "game/psyq.h"
#include "pc_frame.h"
#include "pc_movie.h"
#include "pc_screen.h"
#include "pc_vdec.h"
#include "game/ob.h"
#include "game/vector.h"
#include "game/baddy.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/camera.h"

/* LIBCD.H: the CD audio mix */
struct CdlATV {
	uchar val0, val1, val2, val3;
};

extern int GameFMV_Active;
extern int GameFMV_IsMovie;
extern int GameFMV_FrameBufferCopyActive;
extern int GameFMV_NumSoundSources;
extern ushort GameFMV_Width;
extern ushort GameFMV_Height;
extern uchar GameFMV_Is24Bit;
extern uchar GameFMV_SliceBuffers;
extern uchar GameFMV_CurrentTrack;
extern int GameFMV_VLCBufferSize;
extern int GameFMV_RingBufferSectors;
extern int GameFMV_EndFrame;
extern int GameFMV_MusicScale;
extern int GameFMV_StartSector;
extern int CycleTracks;
extern int currentDecodeBuffer;
extern int currentVlcBuffer;
extern int sendVLC;
extern int alreadyHandledPause;
extern RECT decodeSlice;
extern u_long *vlcBuffer[2];
extern u_long *pRingBuffer;
extern u_long *decodeSliceBuffer[];
extern SMovieDetails movieDetails[];
extern unsigned char MovieOrder[];
extern CVector DAT_800a6388[]; /* the sound sources (GameFMV_AddSoundSource) */
extern int DAT_800d1650;
extern int MIDILEVEL;
extern int DAT_800d1694;
int FUN_8009085c(char *a, char *b);
int FUN_800913c0(char *a);
unsigned char *FUN_800912bc(int, unsigned char *);
int strNextVlc(int);
unsigned int *strNext(void);
void strKickCD(void);
void handleLidOpen(void);
extern "C" {
	/* mdecdecode.c */
	void DecDCTvlcBuild3(void);
	int DecDCTvlc3(unsigned int *, unsigned int *);
}
typedef unsigned char U8;

/* GameFMV_SetScreenTexture__FUi (0x800229ec) */
/* C++ (GAMEFMV.CPP area, after the MDEC decoder). Looks up a texture by checksum and, unless
   GameFMV_Active is set or GGame == 7, clears its VRAM rectangle (the FMV frame
   size, GameFMV_Width x GameFMV_Height) to grey, records its position, and hands the 16-bit tpage
   to GameFMV_RetargetScreenFaces. The arg is passed straight through in a0. */
void GameFMV_SetScreenTexture(uint Checksum) {
	Texture *pTex;
	RECT r;

	GameFMV_StartX = -1;

	if (GameFMV_Active == 0 && GGame != 7) {
		pTex = Spool_FindTextureEntry(Checksum);

		if (pTex) {
			ushort x = pTex->x;
			ushort y = pTex->y;

			GameFMV_StartX = x;
			GameFMV_StartY = y;
			r.x = x;
			r.w = GameFMV_Width;
			r.h = GameFMV_Height;
			r.y = y;
			PC_TexFill(r.x, r.y, r.w, r.h, 0x080808);

			GameFMV_EndX =
			    GameFMV_StartX + (GameFMV_Is24Bit ? (GameFMV_Width * 3) >> 1 : GameFMV_Width);
			GameFMV_RetargetScreenFaces(getTPage(2, 0, GameFMV_StartX, GameFMV_StartY), pTex);
		}
	}
}

/* GameFMV_AddSoundSource__FPC7CVector (0x80022aec) */
void GameFMV_AddSoundSource(const CVector *s) {
	int n;

	if (GameFMV_Active == 0) {
		n = GameFMV_NumSoundSources;

		if (n < 8) {
			DAT_800a6388[n] = *s;
			GameFMV_NumSoundSources = n + 1;
		}
	}
}

/* FUN_80022b4c (0x80022b4c) */
void FUN_80022b4c(void) {
	char buf[24];
	int i;

	for (i = 0; i < 26; i++) {
		if (movieDetails[MovieOrder[i]].startsector) {
			/* PC: the file's stream (pc_stream.h) where the PS1 kept its position on the disc */
			movieDetails[MovieOrder[i]].startsector = PC_StreamFind(movieDetails[MovieOrder[i]].name);
			continue;
			if (FUN_8009085c(buf, movieDetails[MovieOrder[i]].name))
				movieDetails[MovieOrder[i]].startsector = FUN_800913c0(buf);
			else
				movieDetails[MovieOrder[i]].startsector = 0;
		}
	}
}

/* GameFMV_PlayMovie__Fi (0x80022c1c) */
/* Fuzzy demo twin (0.98; retail drops the demo's ExitDemo path), locals from its
   MAIN.SYM. Plays the current FMV: allocates the bitstream ring and slice buffers, switches the
   display to the movie's size (24-bit for hi-colour movies), sets the CD volume, then decodes
   frame by frame until the stream ends, a button is pressed or the lid opens. Then it restores
   the display and frees everything. */
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

	if (!movieDetails[GameFMV_CurrentTrack].startsector)
		return;

	decodeSlice.x = GameFMV_StartX;
	decodeSlice.y = GameFMV_StartY;
	decodeSlice.h = GameFMV_Height;
	decodeSlice.w = GameFMV_Is24Bit ? 24 : 16;
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
		PC_MovieDecode(vlcBuffer[currentVlcBuffer], decodeSlice.x, decodeSlice.y & 255, GameFMV_EndX,
		               decodeSlice.h);

		if (decodeSlice.y < 256)
			decodeSlice.y += 256;
		else
			decodeSlice.y -= 256;

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

			while (decodeSlice.x != GameFMV_StartX) {
				if (!count)
					goto end;

				count--;
			}

			if (!count)
				break;
		}

		SetDefDispEnv(&disp, 0, (decodeSlice.y < 256) << 8, GameFMV_Is24Bit ? 480 : 320, 240);

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

/* GameFMV_SetStartTrack__Fi (0x80023168) */
void GameFMV_SetStartTrack(int a) {
	if (GameFMV_Active)
		return;

	/* the next song is the next entry of Data/Music/Playlist.m3u when there is one */
	if (a >= 0 && PC_MusicNextSong())
		return;

	if (a < 0) {
		a = ~a;
	} else {
		a = CycleTracks++;

		while (CycleTracks < 26) {
			if (movieDetails[CycleTracks].width == 96)
				break;

			CycleTracks++;
		}

		if (CycleTracks == 26)
			CycleTracks = 2;
	}

	GameFMV_CurrentTrack = a;
	GameFMV_Width = movieDetails[a].width;
	GameFMV_Height = movieDetails[a].height;
	GameFMV_EndFrame = movieDetails[a].endframe;
	GameFMV_VLCBufferSize = movieDetails[a].vlcbuffersize;
	GameFMV_RingBufferSectors = movieDetails[a].ringbuffersectors;
	GameFMV_Is24Bit = movieDetails[a].is24bit;
	GameFMV_MusicScale = movieDetails[a].musicScale;
	GameFMV_StartSector = movieDetails[a].startsector;
}

/* GameFMV_StartFrameBufferCopy__Fv (0x80023260) */
void GameFMV_StartFrameBufferCopy(void) {
	GameFMV_FrameBufferCopyActive = 1;
}

/* GameFMV_StartFMV__Fi (0x80023270) */
void GameFMV_StartFMV(int a) {
	int i, j;
	unsigned t, u;

	if (GameFMV_Active == 0 && movieDetails[GameFMV_CurrentTrack].startsector != 0) {
		decodeSlice.x = GameFMV_StartX;
		decodeSlice.y = GameFMV_StartY;
		decodeSlice.w = GameFMV_Is24Bit ? 24 : 16;
		alreadyHandledPause = 0;
		GameFMV_IsMovie = 0;
		currentDecodeBuffer = 0;
		currentVlcBuffer = 0;
		sendVLC = 0;
		decodeSlice.h = GameFMV_Height;

		if (a) {
			for (i = 0; i < 2; i++)
				vlcBuffer[i] = (u_long *)Mem_NewX(GameFMV_VLCBufferSize, 1, 1);
		}

		pRingBuffer = (u_long *)Mem_NewX(GameFMV_RingBufferSectors << 11, 1, 1);
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
					decodeSliceBuffer[j] = (u_long *)Mem_NewX(GameFMV_Height << 5, 1, 1);
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

/* GameFMV_StopFMV__Fv (0x800233d0) */
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
		PC_StreamMix(p);

		GameFMV_Active = 0;
		PC_StreamStop();

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
			PC_TexFill((ushort)r.x, (ushort)r.y, (ushort)r.w, (ushort)r.h, 0x010101);
		}
	}
}

/* GameFMV_SkipTrack__Fv (0x80023528) */
void GameFMV_SkipTrack(void) {
	CVector st;
	CVector *s;

	GameFMV_StopFMV();

	s = &st;
	st.vx = 0;
	s->vy = 0;
	s->vz = 0;
	GameFMV_SetStartTrack(2);
	GameFMV_AddSoundSource(s);
	GameFMV_StartFMV(GameFMV_StartX != -1);
}

/* GameFMV_SyncFMV__Fi (0x8002357c) */
void GameFMV_SyncFMV(int a) {
	int i, best, bi, d;

	if (DAT_800d1650) {
		DAT_800d1650 = DAT_800d1650 - 1;

		if (DAT_800d1650 == 0) {
			bi = 0;
			best = 0xffffff;

			for (i = 0; i < GameFMV_NumSoundSources; i++) {
				d = Utils_CrapDist(CameraList->mPos, DAT_800a6388[i]);

				if (d < best) {
					best = d;
					bi = i;
				}
			}

			GameFMV_SetCDMixForPos(DAT_800a6388[bi]);

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
					                 decodeSlice.x, decodeSlice.y, GameFMV_EndX, decodeSlice.h);

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

/* GameFMV_RetargetScreenFaces__FUsP7Texture (0x800237b4) */
/* C++: points the faces of the environment regions' and environmental objects' models that are
   textured from inside pTex (same tpage/clut, uvs within its box) at the FMV screen's tpage, with
   their uvs halved. The faces (model file data, 32-bit words) are walked by raw offsets: the
   textured face's uv/clut/tpage words at +16 (SFace::TexWord on), its size in words in bits 18+. */
void GameFMV_RetargetScreenFaces(ushort arg0, Texture *a1) {
	unsigned short t9 = arg0;
	int i;
	int m, m2;
	CBody *o;

	i = 0;

	do {

		m = EnvRegions[i];

		if (m != -1) {
			SModel **lst;
			unsigned t8, t6;

			/* the port's model array has no count before it (retail's in-file table did) */
			lst = PSXRegion[m].ppModels;
			t8 = PSXRegion[m].NumModels;
			t6 = 0;

			if (t8 != 0) {
				SModel **t5;

				t5 = lst;

				do {
					char *t2, *t0;
					unsigned t3;
					unsigned short t4;

					t2 = (char *)*t5;
					t3 = 0;
					t0 = t2 + (((SModel *)t2)->NumVertices * 8 + 28) + ((SModel *)t2)->NumNormals * 8;
					t4 = ((SModel *)t2)->NumFaces;

					if (t4 != 0) {
						do {
							unsigned t1;
							char *a2;

							t1 = *(unsigned *)t0;

							if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
								if (*(unsigned short *)(a2 + 6) == a1->tpage &&
								    *(unsigned short *)(a2 + 2) == a1->clut &&
								    *(U8 *)(t0 + 16) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(t0 + 16) &&
								    *(U8 *)(a2 + 4) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(a2 + 4) &&
								    *(U8 *)(a2 + 8) >= a1->u0 &&
								    a1->u1 >= *(U8 *)(a2 + 8) &&
								    *(U8 *)(a2 + 1) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 1) &&
								    *(U8 *)(a2 + 5) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 5) &&
								    *(U8 *)(a2 + 9) >= a1->v0 &&
								    a1->v2 >= *(U8 *)(a2 + 9)) {
									*(unsigned short *)t2 |= 0x200;
									*(unsigned short *)(a2 + 6) = t9;
									*(U8 *)(t0 + 16) = *(U8 *)(t0 + 16) >> 1;
									*(U8 *)(a2 + 4) = *(U8 *)(a2 + 4) >> 1;
									*(U8 *)(a2 + 8) = *(U8 *)(a2 + 8) >> 1;
									*(U8 *)(a2 + 10) = *(U8 *)(a2 + 10) >> 1;
								}
							}

							t0 += (*(unsigned *)t0 >> 18) * 4;
							t3++;
						} while (t3 < t4);
					}

					t6++;
					t5++;
				} while (t6 < t8);
			}
		}

		i++;
	} while (i < 2);

	o = EnvironmentalObjectList;

	if (o) {
		do {

			m2 = o->mRegion;

			if (m2 != -1) {
				SModel **lst;
				unsigned t8, t6;

				lst = PSXRegion[m2].ppModels;
				t8 = PSXRegion[m2].NumModels;
				t6 = 0;

				if (t8 != 0) {
					SModel **t5;

					t5 = lst;

					do {
						char *t2, *t0;
						unsigned t3;
						unsigned short t4;

						t2 = (char *)*t5;
						t3 = 0;
						t0 = t2 + (((SModel *)t2)->NumVertices * 8 + 28) + ((SModel *)t2)->NumNormals * 8;
						t4 = ((SModel *)t2)->NumFaces;

						if (t4 != 0) {
							do {
								unsigned t1;
								char *a2;

								t1 = *(unsigned *)t0;

								if ((t1 & 1) && (a2 = t0 + 16, (t1 & 2))) {
									if (*(unsigned short *)(a2 + 6) == a1->tpage &&
									    *(unsigned short *)(a2 + 2) == a1->clut &&
									    *(U8 *)(t0 + 16) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(t0 + 16) &&
									    *(U8 *)(a2 + 4) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 4) &&
									    *(U8 *)(a2 + 8) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 8) &&
									    *(U8 *)(a2 + 10) >= a1->u0 &&
									    a1->u1 >= *(U8 *)(a2 + 10) &&
									    *(U8 *)(a2 + 1) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 1) &&
									    *(U8 *)(a2 + 5) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 5) &&
									    *(U8 *)(a2 + 9) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 9) &&
									    *(U8 *)(a2 + 11) >= a1->v0 &&
									    a1->v2 >= *(U8 *)(a2 + 11)) {
										*(unsigned short *)t2 |= 0x200;
										*(unsigned short *)(a2 + 6) = t9;
										*(U8 *)(t0 + 16) = *(U8 *)(t0 + 16) >> 1;
										*(U8 *)(a2 + 4) = *(U8 *)(a2 + 4) >> 1;
										*(U8 *)(a2 + 8) = *(U8 *)(a2 + 8) >> 1;
										*(U8 *)(a2 + 10) = *(U8 *)(a2 + 10) >> 1;
									}
								}

								t0 += (*(unsigned *)t0 >> 18) * 4;
								t3++;
							} while (t3 < t4);
						}

						t6++;
						t5++;
					} while (t6 < t8);
				}
			}

			o = (CBody *)o->mNextItem;
		} while (o);
	}
}

/* handleLidOpen__Fv (0x80023c48) */
void handleLidOpen(void) {
	unsigned char res[8];

	do
		CdControlB(1, 0, res);

	while (res[0] & 0x10);

	CdControlB(19, 0, 0);

	while (!(res[0] & 2))
		CdControlB(1, 0, res);
}

/* strInit__Fv (0x80023cb8) */
void strInit(void) {

	Redbook_XACheckSectorOnVSync = 0;
	DAT_800d169c = 1;
	Redbook_XAModeSet = 0;
	strKickCD();
	PC_StreamMute(0);
}

/* strCallback__Fv (0x80023d3c) */
void strCallback(void) {
	RECT r;
	u_long *p;

	return;

	r = decodeSlice;
	p = decodeSliceBuffer[currentDecodeBuffer];
	currentDecodeBuffer++;

	if (currentDecodeBuffer >= GameFMV_SliceBuffers)
		currentDecodeBuffer = 0;

	decodeSlice.x += ((unsigned short)decodeSlice.w);

	if (decodeSlice.x < (ushort)GameFMV_EndX) {
		DecDCTout(decodeSliceBuffer[currentDecodeBuffer], (short)((unsigned short)decodeSlice.w) * decodeSlice.h / 2);
	} else {
		decodeSlice.x = GameFMV_StartX;

		if (GameFMV_IsMovie) {
			if (decodeSlice.y < 256)
				decodeSlice.y += 256;
			else
				decodeSlice.y -= 256;
		}
	}

	if (GameFMV_StartX != -1)
		LoadImage(&r, p);
}

/* strNext__Fv (0x80023e88) */
unsigned int *strNext(void) {
	uint32_t *addr;
	uint32_t *hdr;

	if (!PC_StreamNextFrame(&addr, &hdr))
		return 0;

	GameFMV_CurrentFrame = hdr[2];

	if (GameFMV_CurrentFrame >= GameFMV_EndFrame)
		GameFMV_RewindSwitch = 1;

	return (unsigned int *)addr;
}

/* strNextVlc__Fi (0x80023edc) */
int strNextVlc(int f) {
	unsigned int *s = strNext();

	if (!s)
		return 0;

	if (f)
		DecDCTvlc3(s, (unsigned int *)vlcBuffer[currentVlcBuffer]);

	PC_StreamFreeFrame(s);
	return f;
}

/* strKickCD__Fv (0x80023f44) */
void strKickCD(void) {
	unsigned char loc[4];
	unsigned char param[4];

	/* PC: GameFMV_StartSector is the file's stream (pc_stream.h) */
	PC_StreamPlay(GameFMV_StartSector);
	return;
	param[0] = 0x80;
	FUN_800912bc(GameFMV_StartSector, loc);

	do {
		while (!CdControl(2, loc, 0))
			;

		while (!CdControl(14, param, 0))
			;
	} while (!CdRead2(480));
}

/* GameFMV_ScaleMusicVolume__Fi (0x80023fb0) */
int GameFMV_ScaleMusicVolume(int a) {
	int t = (a * GameFMV_MusicScale) / 128;

	if (t > 16383)
		t = 16383;

	return t;
}

/* FUN_80023fe8 (0x80023fe8) */
void FUN_80023fe8(void) {
	int i;

	for (i = 7; i != -1; i--)
		;
}

/* Redbook_XAAllow__Fb (0x80024004) */
/* (retail's parameter is a bool; the C source's int keeps its code: a bool would be
   zero-extended from its byte) */
void Redbook_XAAllow(int v) {
	DAT_800d1694 = v;
}

/* GameFMV_SetCDMixForPos__FRC7CVector (0x80024010) */
/* Sets the CD-audio mixer for a sound at pos: the spatial attenuation's left/right
   (12 bits each) are scaled by the current volume and added to a 3/4-volume base. */
void GameFMV_SetCDMixForPos(const CVector &pos) {
	CdlATV atv;
	unsigned int att = Utils_CalculateSpatialAttenuation(pos, 1024, 16384);
	int vol = GameFMV_ScaleMusicVolume(XALEVEL);
	int l = ((att & 0xfff) * vol) >> 18;
	int r = (((att >> 16) & 0xfff) * vol) >> 18;
	int base;

	if (l > 255)
		l = 255;

	if (r > 255)
		r = 255;

	base = (vol * 192) >> 14;
	l = base + l / 4;
	r = base + r / 4;
	atv.val0 = l;
	atv.val1 = r;
	atv.val3 = 0;
	atv.val2 = 0;

	PC_StreamMix(&atv.val0);
}
