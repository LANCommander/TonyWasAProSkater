/* MAIN.CPP: the main loop - the per-frame game logic and display, the FPS counter, game over,
   creating the skaters and cameras, PlayAway and LaunchGame (declarations in game/main.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "pc_screen.h"
#include "game/main.h"
#include "game/class.h"
#include "game/m3d.h"
#include "game/career.h"
#include "game/bruce.h"
#include "game/types.h"
#include "pc_texstore.h"
#include "game/bit.h"
#include "game/camera.h"
#include "game/fileio.h"
#include "game/flash.h"
#include "game/font.h"
#include "game/front.h"
#include "game/mess.h"
#include "game/ob.h"
#include "game/pad.h"
#include "game/panel.h"
#include "game/shatter.h"
#include "game/spool.h"
#include "game/trickob.h"
#include "game/tricktips.h"
#include "game/utils.h"
#include "game/dbuffer.h"
#include "game/padbuttons.h"
#include <stdio.h>
#include <stdlib.h>
#include "game/psyq.h"
#include "game/video.h"
#include "game/powerup.h"
#include "game/card.h"
#include "game/sfx.h"
#include "game/shellutils.h"
#include "game/crowd.h"
#include "game/db.h"
#include "game/gamefmv.h"
#include "game/hires.h"
#include "game/rfront.h"
#include "game/init.h"
#include "game/trig.h"

#include "game/mem.h"
#include "game/backgrnd.h"
#include "game/globals.h"
#include "game/pack.h"
#include "game/pal.h"
#include "game/physics.h"
#include "game/shell.h"
#include "game/frontend.h"
#include "game/rail.h"

extern int GAutoLevela, GAutoPlayer1a, GAutoPlayer2a, GAutoGamea;
#define POLYBUFFERSIZE (Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00)
extern int GSpinSpeed;
extern int Mick_Darkness;
extern int EveryOther;
extern int DAT_800d183c;
extern int DAT_800d1840;
struct SEnviro;
extern int Mick_MapMode, Mick_MapDoit;
extern "C" void FUN_8002efa4(CBruce *);
extern SViewport LeftViewport;
extern SViewport RightViewport;
extern SViewport TopViewport;
extern SViewport BottomViewport;
extern SViewport TopLeftViewport;
extern SViewport BottomRightViewport;
extern SViewport TopLeftBigViewport;
extern SViewport BottomRightBigViewport;
extern char *DAT_800d16dc;
extern int PolyBufferBytesUsed;
extern int DAT_800d1748;
extern int CloseCam;
void BlackOut(int, int, int, int);
void Mick_Darken(void);
void Display_Viewport(SViewport *, int);
void FUN_8006dc78(void);
short FUN_8007b0fc(void *);
extern int DAT_800a7214[];
extern int LastXblanks;
extern char DAT_800d2580[];
extern int DAT_800d1750;
extern int XFramesShifted;
extern int DAT_800d2230;
extern char InShell;
extern int DAT_800d1f18;
extern "C" void FUN_8007ef30(void);
void ReplaceTexture(uint, char *);
extern uchar DAT_800a8245[];
extern char GVideoFlags;
extern "C" void PC_RunStaticCtors(void);
enum EPackInitType { PACK_0, PACK_1 }; /* (INIT.CPP's) */
extern char DAT_8009f5cc[];
void DumpAutoTest();

/* DumpAutoTest__Fv (0x800240d8) */
void DumpAutoTest(void) {
	char pad[24];
	char env[96];
	char buf[200];
	int id;

	/* PC: libgpu's debug font isn't drawn; the line goes to the trace log */
	PC_ScreenEnable(1);
	sprintf(buf, "L-%d, P1-%d, P2-%d, G-%d", GAutoLevela, GAutoPlayer1a, GAutoPlayer2a, GAutoGamea);
	printf("%s\n", buf);

	GAutoLevela = GAutoLevel;
	GAutoPlayer1a = GAutoPlayer1;
	GAutoPlayer2a = GAutoPlayer2;
	GAutoGamea = GAutoGame;
}

/* ResetMainDemoTimer__Fv (0x800241d4) */
void ResetMainDemoTimer(void) {}

/* __dl__6CClassPv (0x80024238) */
/* CClass's operator delete: frees through the memory manager. */
void CClass::operator delete(void *p) {
	Mem_Delete(p);
}

/* CalcPolyBufferEnd__Fv (0x80024258) */
/* CalcPolyBufferEnd: the demo's MAIN.CPP twin used a fixed POLYBUFFERSIZE; THPS1 sizes the
   primitive buffer per level and per player count. */
void CalcPolyBufferEnd(void) {
	unsigned char *Prims = pDoubleBuffer->Polys;
	int Size = DB_PACKET_MEMORY(POLYBUFFERSIZE);

	PolyBufferEnd = Prims + Size - 256;
}

/* PollHost__Fv (0x800242cc) */
void PollHost(void) {}

/* Game_Over__Fv (0x800242d4) */
/* C++: the skaters are CBruce objects (their fields move in a 64-bit build) */
void Game_Over() {
	int a, b, c;

	if (GGame == 8) {
		a = DAT_800d17cc->mGraffitiCount;
		b = GSkater[0]->mGraffitiCount;

		if (a != b) {
			if (b < a) {
				GWins2++;
				GLosses1++;

				if (GStreak2 < 0)
					GStreak2 = 1;
				else
					GStreak2++;

				GStreak1 = -GStreak2;
			} else {
				GWins1++;
				GLosses2++;

				if (GStreak1 < 0)
					GStreak1 = 1;
				else
					GStreak1++;

				GStreak2 = -GStreak1;
			}

			c = GSkater[0]->mGraffitiCount;

			if (GHighTags1 < c)
				GHighTags1 = c;

			c = DAT_800d17cc->mGraffitiCount;

			if (GHighTags2 < c)
				GHighTags2 = c;
		}
	}

	if (GState == 0) {
		GState = 1;
		GSpin = 80;
		GSpinSpeed = 64;

		if (DAT_800d17cc)
			DAT_800d17cc->mLeanAngle = 0;
	}
}

/* Mick_Darken__Fv (0x80024454) */
/* Fuzzy demo twin; locals from its MAIN.SYM. Fades a full-screen semi-transparent
   black-ish quad in (brightness climbs by 4 a frame up to 40). The corners are set in the demo's
   source-line grouping (x0/x1, y0/y1, x2/x3, y2/y3); setXY4's order schedules differently. */
void Mick_Darken() {
	POLY_F4 *aPoly;
	DR_TPAGE *ptpage;
	uchar *newppoly;
	P_TAG *ot = &pDoubleBuffer->OT;

	if (Mick_Darkness < 40)
		Mick_Darkness += 4;

	aPoly = (POLY_F4 *)pPoly;
	newppoly = (uchar *)aPoly + sizeof(POLY_F4) + sizeof(DR_TPAGE);
	ptpage = (DR_TPAGE *)(aPoly + 1);

	if (newppoly > PolyBufferEnd)
		return;

	pPoly = newppoly;
	setPolyF4(aPoly);
	setSemiTrans(aPoly, 1);
	setRGB0(aPoly, Mick_Darkness, Mick_Darkness, Mick_Darkness);

	aPoly->x0 = 0;
	aPoly->x1 = 512;
	aPoly->y0 = 0;
	aPoly->y1 = 0;
	aPoly->x2 = 0;
	aPoly->x3 = 512;
	aPoly->y2 = 240;
	aPoly->y3 = 240;
	addPrim(ot, aPoly);

	setlen(ptpage, 1);
	ptpage->code[0] = _get_mode(1, 1, 0x40);
	addPrim(ot, ptpage);
}

/* DownloadBMP__FPciiii (0x80024564) */
void DownloadBMP(char *pData, int x, int y, int ClutX, int ClutY) {
	int w;
	int h;
	int colors;
	int transparent_color;
	ushort colorTab[256];
	char *pColor;
	RECT r;
	int pad;
	char *data;
	char *in;
	char *out;

	w = *(ushort *)(pData + 18);
	h = *(ushort *)(pData + 22);
	colors = *(ushort *)(pData + 46);

	if (colors == 0)
		colors = 256;

	pColor = pData + 54;
	transparent_color = 0;

	{
		int i;

		for (i = 0; i < colors; i++) {
			ushort r, g, b;

			b = (unsigned char)pColor[0];
			g = (unsigned char)pColor[1];
			r = (unsigned char)pColor[2];

			if (b >= 248 && g == 0 && r >= 248) {
				transparent_color = i;
				colorTab[i] = 0;
			} else {
				b >>= 3;
				g >>= 3;
				r >>= 3;

				if (b == 0 && g == 0 && r == 0)
					colorTab[i] = 0x8000;
				else
					colorTab[i] = (b << 10) + (g << 5) + r;
			}

			pColor += 4;
		}
	}

	r.x = ClutX;
	r.y = ClutY;
	r.w = 256;
	r.h = 1;
	PC_TexUpload(r.x, r.y, r.w, r.h, colorTab);
	DrawSync(0);

	pad = w % 4;

	if (pad > 0)
		pad = 4 - pad;

	w += pad;
	data = (char *)Mem_New((w + 3) * h, 1, 1);
	in = pData + 54 + colors * 4;
	out = data + (h - 1) * w;

	{
		int j;
		int i;

		for (j = 0; j < h; j++) {
			{
				int i;

				for (i = 0; i < w - pad; i++)
					*out++ = *in++;
			}

			{
				int i;

				for (i = 0; i < pad; i++) {
					*out++ = transparent_color;
					in++;
				}
			}

			out -= w * 2;
		}
	}

	r.x = x;
	r.y = y;
	r.w = w / 2;
	r.h = h;
	PC_TexUpload(r.x, r.y, r.w, r.h, data);
	DrawSync(0);
	Mem_Delete(data);
}

/* ReplaceTexture__FUiPc (0x800247a8) */
void ReplaceTexture(unsigned id, char *name) {
	Texture *e = Spool_FindTextureEntry(id);

	if (e) {
		unsigned char *info = (unsigned char *)e->pPalette;
		int pal = Pal256Y;
		int w = e->x;
		int h = e->y;
		int k;
		char *bmp;

		pal += info[2]; /* the palette's slot */
		k = Pal256X;
		bmp = (char *)Mem_NewX(FileIO_Open(name), 1, 1);
		FileIO_Load(bmp);
		FileIO_Sync();
		DownloadBMP(bmp, w, h, k, pal);
		Mem_Delete(bmp);
	}
}

/* Game_Logic__Fv (0x80024868) */
/* Fuzzy demo twin; locals (Frozen) from MAIN.SYM. Runs the per-frame game logic
   on every other frame: objects, tricks, panels, front end and messages, swapping the two pads
   around the front-end update in two-player mode. */
void Game_Logic(void) {
	int Frozen;

	DAT_800d183c++;

	if (EveryOther++ & 1)
		return;

	TTime++;
	Font_Reset();

	Frozen = 0;

	if (GamePaused || GameFrozen)
		Frozen = 1;

	if (!Frozen) {
		Flash_Update();
		Utils_ClearExplosionStack();
		Ob_ClearInputFlags(BaddyList);
		Ob_ClearInputFlags(EnvironmentalObjectList);
		Ob_AI(&MechList);

		if (!GamePaused) {
			Bit_Move();
			Rail_Update();
			Ob_AI(&PowerUpList);
			TrickOb_Update();
			Ob_QuickAI(&EnvironmentalObjectList);
			Ob_QuickAI(&BackgroundList);
			Ob_MaybeUnSuspendOrCull();
			Ob_AI(&BaddyList);
			Bit_RemoveDeadBits();

			if (GGame != 7)
				if (GState == 1 && GSpin < 40 && !GSkater[0]->mPhysicsState &&
				    (!DAT_800d17cc || !DAT_800d17cc->mPhysicsState)) {
					GSkater[0]->mpCamera->extra_YRot += GSpinSpeed;

					if (DAT_800d17cc)
						DAT_800d17cc->mpCamera->extra_YRot += GSpinSpeed;
				}
		}
	} else {
		if (GState == 1)
			Ob_AI(&CameraList);
	}

	Shatter_MaybeMakeGlassShatterSound();
	Panel_Update();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	Pad_Remap();
	Front_Update();
	Pad_Restore();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	TrickTips_Update();
	Mess_Update();
	Spool_Continue();
	FileIO_Continue();
}

/* Display_Viewport__FP9SViewporti (0x80024ce4) */
/* MAIN.CPP (demo twin). Renders one viewport: points the OT at its layer block, sets up the
   camera, then renders the item lists, skaters (with their shadow tint), bits and background. */
void Display_Viewport(SViewport *a, int b) {
	short s1, s0;

	Camera_CurrentMikeCamera = b;
	M3d_RenderSetup(&MikeCamera[b], a, pDoubleBuffer->pOT = &pDoubleBuffer->OT3D[b * 4096]);

	Mick_MapDoit = Mick_MapMode;
	M3d_Render(EnviroList);

	Mick_MapDoit = 0;
	M3d_Render(EnvironmentalObjectList);
	FUN_8002efa4(GSkater[0]);

	if (DAT_800d17cc)
		FUN_8002efa4(DAT_800d17cc);

	s1 = M3d_OTPushback;
	s0 = M3d_OTPushback2;

	if (GSkater[0]->mPhysicsState && GSkater[0]->mInAir == 0) {
		M3d_OTPushback = 128;
		M3d_OTPushback2 = 128;
	} else {
		M3d_OTPushback = 0;
		M3d_OTPushback2 = 0;
	}

	M3d_Render(MechList);
	GSkater[0]->TempRestore();

	if (DAT_800d17cc)
		DAT_800d17cc->TempRestore();

	M3d_OTPushback = s1;
	M3d_OTPushback2 = s0;
	Bit_Display();
	M3d_Render(BaddyList);
	M3d_Render(PowerUpList);

	if (ViewportMode == 0)
		M3d_RenderBackground(BackgroundList);

	M3d_RenderCleanup();
}

/* Draw_Rect__Fiiiiiiiii (0x80024e98) */
/* Demo twin; locals from its MAIN.SYM. Adds a flat quad with the given corners
   (right and bottom edges inclusive) and packed colour word to ordering-table slot 2. */
void Draw_Rect(int X0, int Y0, int X1, int Y1, int X2, int Y2, int X3, int Y3, int c) {
	POLY_F4 *pF4 = (POLY_F4 *)pPoly;
	uchar *newppoly;

	*(u_long *)&pF4->r0 = c;
	pF4->x0 = X0;
	pF4->y0 = Y0;
	pF4->x1 = X1 + 1;
	pF4->y1 = Y1;
	pF4->x3 = X2 + 1;
	pF4->y3 = Y2 + 1;
	pF4->x2 = X3;
	pF4->y2 = Y3 + 1;
	newppoly = (uchar *)pF4 + sizeof(POLY_F4);
	SetPolyF4((POLY_F4 *)pPoly);
	addPrim(&pDoubleBuffer->OT3D[0], pPoly);

	pPoly = newppoly;
}

/* BlackOut__Fiiii (0x80024f64) */
void BlackOut(int x, int y, int w, int h) {
	Draw_Rect(x, y, x + w, y, x + w, y + h, x, y + h, 0x40000000);
}

/* Display__Fv (0x80024fa4) */
void Display(void) {
	int f, a, b, c;

	if (DAT_800d16dc) {
		Mess_SetRGB(255, 0, 0, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(0);
		Mess_DrawText(256, 60, DAT_800d16dc, 0, 0, 256, 0);
	}

	ApocalypseViewport.Zoom = FUN_8007b0fc(CameraList);

	if (!(GameFrozen && Cheat_ScreenShots == 0)) {
		/* (the C source passed GameFrozen too) */
		((void (*)(CBody **, int))Ob_AI)(&CameraList, GameFrozen);

		if (Cheat_ScreenShots)
			goto L90;
	}

	if (Pad[0].Camera.Pressed) {
		if (DAT_800d1748) {
			DAT_800d1748 = 0;

			if (GNumberOfPlayers == 2 && GGame != 7) {
				if (++ViewportMode >= 4)
					ViewportMode = 0;
			} else
				CloseCam ^= 1;
		}
	} else
		DAT_800d1748 = 1;

	if (Pad[1].Camera.Triggered) {
		Pad[1].Camera.Triggered = 0;

		if (++ViewportMode >= 4)
			ViewportMode = 0;
	}
L90:
	if (GNumberOfPlayers == 2 && ViewportMode == 0 && GGame != 7)
		ViewportMode = 1;

	switch (ViewportMode) {
		case 0:
			f = GameFrozen;

			if (f == 0) {
				if (CloseCam) {
					c = 386;
					a = 30;
					camDist = c;
					b = -160;
				} else {
					c = 290;
					a = 150;
					camDist = c;
					b = -108;
				}

				camPitch = a;
				camYOffset = b;
			}

			Display_Viewport(&ApocalypseViewport, 0);
			break;
		case 1:
			f = GameFrozen;

			if (f == 0) {
				camDist = 286;
				camPitch = 118;
				camYOffset = -140;
			}

			BlackOut(254, 0, 4, 240);
			Display_Viewport(&LeftViewport, 0);
			Display_Viewport(&RightViewport, 1);
			break;
		case 2:
			f = GameFrozen;

			if (f == 0) {
				camDist = 450;
				camPitch = 60;
				camYOffset = -120;
			}

			BlackOut(0, 118, 512, 4);
			Display_Viewport(&TopViewport, 0);
			Display_Viewport(&BottomViewport, 1);
			break;
		case 3:
			f = GameFrozen;

			if (f == 0) {
				camDist = 642;
				camPitch = 32;
				camYOffset = -172;
			}

			BlackOut(256, 0, 256, 120);
			BlackOut(0, 120, 256, 120);
			Display_Viewport(&TopLeftViewport, 0);
			Display_Viewport(&BottomRightViewport, 1);
			break;
		case 4:
			f = GameFrozen;

			if (f == 0) {
				camDist = 610;
				camPitch = 0;
				camYOffset = -200;
			}

			BlackOut(304, 0, 208, 88);
			BlackOut(0, 152, 208, 88);
			Display_Viewport(&TopLeftBigViewport, 0);
			Display_Viewport(&BottomRightBigViewport, 1);
			break;
	}

	Font_Display();
	FUN_8006dc78();
	Front_Display();
	Mess_Display();

	if (GamePaused && Status != 33)
		Mick_Darken();
	else
		Mick_Darkness = 0;

	Flash_Display();

	PolyBufferBytesUsed = pPoly - pDoubleBuffer->Polys;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		extern unsigned char *PolyBufferEnd;
		fprintf(stderr, "polybuf f%u used=%d cap=%d remaining=%d\n", pc_heartbeat,
		        PolyBufferBytesUsed,
		        (int)(PolyBufferEnd - pDoubleBuffer->Polys), (int)(PolyBufferEnd - pPoly));
	}
}

/* FPS_Init__Fv (0x80025400) */
void FPS_Init(void) {
	int i;

	for (i = 0; i < 3; i++)
		DAT_800a7214[i] = 2;

	LastXblanks = Xblanks - 2;
}

/* FPS_Display__Fv (0x80025440) */
/* (Xblanks read as a plain int, as the C source did: a volatile read reorders the sum) */
void FPS_Display(void) {
	int a1, i, s1, t;

	if (GamePaused == 0) {
		t = DAT_800d1750 + 1;
		DAT_800a7214[DAT_800d1750] = *(int *)&Xblanks - LastXblanks;
		DAT_800d1750 = t;

		if (t == 3)
			DAT_800d1750 = 0;
	}

	a1 = 0;
	i = 0;
	LastXblanks = *(int *)&Xblanks;

	do {
		a1 += DAT_800a7214[i];
		i++;
	} while (i < 3);

	s1 = 180 / a1;

	if (a1 >= 16)
		a1 = 15;

	if (GamePaused == 0) {
		TimeScale = (a1 << 8) / 6;

		if (Cheat_SlowMo)
			TimeScale = TimeScale / 4;

		XFramesShifted = XFramesShifted + TimeScale * 2;
		XFrames = XFramesShifted >> 8;
		TimeScaleSquared = (TimeScale * TimeScale) >> 8;
	}

	if (Cheat_FPS) {
		DAT_800d2230 = (DAT_800d2230 + 10) & 127;
		sprintf(DAT_800d2580, " %d FPS", s1);

		if (GameFrozen == 0) {
			if (s1 >= 25)
				Font_String(450, 210, DAT_800d2580, 50, 100, 50, 1);
			else if (s1 >= 22)
				Font_String(450, 210, DAT_800d2580, 100, 100, 50, 1);
			else
				Font_String(450, 210, DAT_800d2580, (DAT_800d2230 + 50) & 255, 50, 50, 1);

			Font_Display();
		}
	}
}

/* PlayAway_Init__Fv (0x8002566c) */
/* PlayAway_Init (demo twin PlayAway_Init). LastDeck[] picks each player's board texture,
   LastWheel[] the W1*.bmp colour; reading LastWheel through GCareer (not as a separate global)
   fixes the loop's preheader order (sp+16 hoisted before sp+48). */
void PlayAway_Init() {
	char nm[2][16];
	int i, j, k;

	GClear1 = 0;
	GClear2 = 0;
	strcpy(nm[0], Stats[SelectedSkater].Boards[(uchar)GCareer.LastDeck[0]].File);
	strcpy(nm[1], Stats[SelectedSkater2].Boards[(uchar)GCareer.LastDeck[1]].File);

	for (i = 0; i < 2; i++)
		nm[i][1] = 95;

	ReplaceTexture(Stats[SelectedSkater].BoardChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].BoardChecksum, nm[1]);

	{
		char *tab[15] = { "W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp",
		                  "W1brow01.bmp", "W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp",
		                  "W1gree03.bmp", "W1oran01.bmp", "W1purp01.bmp", "W1red_01.bmp",
		                  "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp" };

		for (j = 0; j < 2; j++) {
			strcpy(nm[j], tab[(uchar)GCareer.LastWheel[j]]);

			nm[j][1] = 95;
		}
	}

	ReplaceTexture(Stats[SelectedSkater].WheelChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].WheelChecksum, nm[1]);

	for (k = 0; k < 2; k++)
		nm[k][1] = 84;

	ReplaceTexture(Stats[SelectedSkater].TruckChecksum, nm[0]);

	if (GNumberOfPlayers == 2)
		ReplaceTexture(Stats[SelectedSkater2].TruckChecksum, nm[1]);

	ApocalypseViewport.TanHalf = 3413;
	LeftViewport.TanHalf = 2275;
	RightViewport.TanHalf = 2275;
	TopViewport.TanHalf = 3413;
	BottomViewport.TanHalf = 3413;
	TopLeftViewport.TanHalf = 2275;
	BottomRightViewport.TanHalf = 2275;
	TopLeftBigViewport.TanHalf = 2275;
	GState = 0;
	BottomRightBigViewport.TanHalf = 2275;
	Vblanks = 0;
	Xblanks = 0;
	XFramesShifted = 0;
	XFrames = 0;
	TTime = 0;
	EveryOther = 0;
	InShell = 0;
	LastXblanks = Xblanks;
	Pad_ClearAll();

	DAT_800d1f14 = 0;
	DAT_800d1f18 = 0;
	DAT_800d183c = 0;
	DAT_800d1840 = 0;
	EndCode = 0;
	Utils_InitialRand(0x12a6cc58);
	FUN_8007ef30();

	GSkaterRegion = Mess_LoadFont("time.fnt", -1, -1, 0);
	DAT_800d1798 = Mess_LoadFont("score.fnt", -1, -1, 0);
	GStartTime = Mess_LoadFont("ingame1.fnt", 2, 2, -1);
}

/* CreateSkatersAndCameras__Fv (0x80025a88) */
void CreateSkatersAndCameras(void) {
	CBruce *pBruce;
	CCamera *pCam;
	CBruce *p2;
	CCamera *c2;

	pBruce = new CBruce();

	GSkater[0] = pBruce;
	DAT_800d17cc = 0;
	pBruce->mPadIndex = 0;
	pCam = new CCamera(pBruce, 0);

	pCam->mCameraMode = (ECameraMode)1;
	TrickTips_Init();

	if (Levels[GLevel].isCompetition && GGame == 1 && GVideoRestart.NumSkaters == 0 && GVideoReplay == 0)
		pCam->StartCompetitionIntro();

	if (GGame != 7 || GHorsePlayer == 0)
		pBruce->mSkaterIndex = SelectedSkater;
	else
		pBruce->mSkaterIndex = SelectedSkater2;

	pBruce->mpCamera = pCam;

	if (GNumberOfPlayers == 2 && GGame != 7)
		pBruce->InitItem(Stats[pBruce->mSkaterIndex].PSXName2);
	else
		pBruce->InitItem(Stats[pBruce->mSkaterIndex].PSXName);

	/* stance: Regular */
	if (*Stats[pBruce->mSkaterIndex].pBio[4] == 'R' || *Stats[pBruce->mSkaterIndex].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;

	pBruce->mpStatBonus = (uchar *)GBoardStats;
	pBruce->GetStats(0);

	if (Stats[pBruce->mSkaterIndex].ComboID == 16384)
		pBruce->mSubObjectDisplayMask |= pBruce->MuskaPartMask();

	if (GNumberOfPlayers == 2 && GGame != 7) {
		p2 = new CBruce();

		p2->mPadIndex = 1;
		c2 = new CCamera(p2, 0);

		c2->mCameraMode = (ECameraMode)1;
		p2->mSkaterIndex = SelectedSkater2;
		p2->InitItem(Stats[SelectedSkater2].PSXName2);

		if (*Stats[p2->mSkaterIndex].pBio[4] == 'R' || *Stats[p2->mSkaterIndex].pBio[4] == 'r')
			p2->mExtraFlags |= 2;

		pBruce->mpOtherSkater = p2;
		p2->mpOtherSkater = pBruce;
		p2->mpCamera = c2;
		DAT_800d17cc = p2;
		p2->mpStatBonus = DAT_800a8245;
		p2->GetStats(1);
	}
}

static inline void System_Logic(void) {
	SControl TempPad;

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	Pad_Remap();
	Pad_Update();
	Pad_Restore();

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	if (GClear1)
		Pad_ClearAllOne(0);

	if (GClear2)
		Pad_ClearAllOne(1);
}

/* PlayAway__Fv (0x80025e00) */
/* PlayAway (called from LaunchGame). The demo splits this into Game_Init (the
   fuzzy twin of the first 490 words), System_Logic and a 216-byte PlayAway loop. THPS1 writes the
   init out in place: its error exits `goto done` straight to the shutdown, which keeps the first
   EndCode test in the init's last block. System_Logic is an inline. Sets up the level (video restart/replay loading, the
   powerup visibility, settling the cameras), runs the frame loop until EndCode is set, then shuts
   the game down and unloads the fonts. */
void PlayAway(void) {
	int XStart;

	PlayAway_Init();

	XStart = Xblanks;
	Pause(1);
	FPS_Init();
	Crowd_Init();
	Crowd_Start();

	EndCode = 0;
	ResetMainDemoTimer();

	if (GVideoRestart.NumSkaters > 0 && DemoTimer) {
		int LoadError;

		Front_SetLoadAction(LOAD_REPLAY);

		VblankUpdateBar = 1;

		if (GVideoRestart.NumSkaters == 1)
			LoadError = Card_Load(1, GVInfo1[0].pBlockStart, 0);
		else
			LoadError = Card_Load(1, GVInfo1[0].pBlockStart, GVInfo1[1].pBlockStart);

		VblankUpdateBar = 0;

		if (LoadError) {
			Mess_SetCurrentFont("ingame1.fnt");
			HiRes_ClearOTag();
			ResetOT();
			SetClippingBox(0, 0, 512, 240);
			DrawColoredBox(0, 0, 512, 240, 0, 0, 0);
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 110, (char *)"load failed", 1, 0, 256, 0);
			Mess_DrawText(256, 130, "press x to continue", 1, 0, 256, 0);
			HiRes_DrawOTag();
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			while (!Pad[0].MenuAccept.Triggered && !Pad[1].MenuAccept.Triggered &&
			       !Pad[0].MenuStart.Triggered && !Pad[1].MenuStart.Triggered)
				Pad_Update();

			EndCode = 9;
			goto done;
		}

		{
			SVideoHead *VH = (SVideoHead *)(GVInfo1[0].pBlockStart + 128);

			GVInfo1[0].NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1[0].Highlights[i] = VH->Highlights[i];
		}

		if (GVideoRestart.NumSkaters == 2) {
			SVideoHead *VH = (SVideoHead *)(GVInfo1[1].pBlockStart + 128);

			GVInfo1[1].NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1[1].Highlights[i] = VH->Highlights[i];
		}

		GVideoRestart.NumSkaters = -GVideoRestart.NumSkaters;
		GVideoReplay = -1;
	}

	if (!DemoTimer) {
		int LoadError;

		VblankUpdateBar = 1;
		LoadError = Card_LoadReplayFromCD(GVInfo1[0].pBlockStart, GVInfo1[1].pBlockStart, DemoName);
		DemoName[4]++;

		if (DemoName[4] > 'J')
			DemoName[4] = 'A';

		VblankUpdateBar = 0;

		if (LoadError) {
			EndCode = 9;
			goto done;
		}

		{
			SVideoHead *VH = (SVideoHead *)(GVInfo1[0].pBlockStart + 128);

			GVideoRestart.NumSkaters = VH->NumSkaters;
			GVideoRestart.Skater = VH->Skater;
			GVideoRestart.Skater2 = VH->Skater2;
			GVideoRestart.Level = VH->Level;
			GVideoRestart.GGame = VH->GGame;
			GVInfo1[0].NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1[0].Highlights[i] = VH->Highlights[i];
		}

		if (GVideoRestart.NumSkaters == 2) {
			SVideoHead *VH = (SVideoHead *)(GVInfo1[1].pBlockStart + 128);

			GVInfo1[1].NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1[1].Highlights[i] = VH->Highlights[i];
		}

		GVideoRestart.NumSkaters = -GVideoRestart.NumSkaters;
		GVideoReplay = -1;
	}

	if (!GVideoReplay) {
		if (GGame == 1) {
			GVideoFlags = 0;

			if (Career_Got(3))
				GVideoFlags |= 1;

			if (Career_Got(2))
				GVideoFlags |= 2;
		} else {
			GVideoFlags = 3;
		}
	} else {
		GVideoFlags = ((SVideoHead *)(GVInfo1[0].pBlockStart + 128))->VideoFlags;
	}

	if (GGame == 1 && !Levels[GLevel].isCompetition) {
		if (GVideoFlags & 2) {
			GPowS->mLifetime = 1;
			GPowK->mLifetime = 1;
			GPowA->mLifetime = 1;
			GPowT->mLifetime = 1;
			GPowE->mLifetime = 1;
		}

		if (GVideoFlags & 1)
			GPowTape->mLifetime = 1;
	}

	if (GVideoReplay) {
		CBody *pSkater = MechList;
		CBody *pSkater2 = (CBody *)pSkater->mNextItem;

		if (GVideoReplay > 0) {
			GVInfo1[0].ReplayHighlight = GVideoReplay;
			GVInfo1[1].ReplayHighlight = GVideoReplay;
			Video_Start((CBruce *)pSkater, (EVideoMode)2,
			            GVInfo1[0].Highlights[GVideoReplay - 1].FromFrame,
			            GVInfo1[0].Highlights[GVideoReplay - 1].ToFrame);

			if (pSkater2)
				Video_Start((CBruce *)pSkater2, (EVideoMode)2,
				            GVInfo1[0].Highlights[GVideoReplay - 1].FromFrame,
				            GVInfo1[0].Highlights[GVideoReplay - 1].ToFrame);

			Video_SyncObsIfNeeded();
		} else {
			GVideoReplay = 0;
			GVInfo1[0].ReplayHighlight = 0;
			GVInfo1[1].ReplayHighlight = 0;
			Video_Start((CBruce *)pSkater, (EVideoMode)2, 1, 0x7fffffff);

			if (pSkater2)
				Video_Start((CBruce *)pSkater2, (EVideoMode)2, 1, 0x7fffffff);
		}

		GState = 0;
		Flash_Reset();

		((CBruce *)MechList)->mTime = Xblanks + 216030;

		if (MechList->mNextItem)
			((CBruce *)MechList->mNextItem)->mTime = Xblanks + 216030;
	} else {
		Video_Cleanup();

		{
			CBody *pSkater = MechList;
			CBody *pSkater2 = (CBody *)pSkater->mNextItem;

			if (!GVInfo1[0].VideoMode && !((CCamera *)CameraList)->InCompetitionIntro()) {
				Video_Start((CBruce *)pSkater, (EVideoMode)1, 1, 0x7fffffff);

				if (pSkater2)
					Video_Start((CBruce *)pSkater2, (EVideoMode)1, 1, 0x7fffffff);

				((SVideoHead *)(GVInfo1[0].pBlockStart + 128))->VideoFlags = GVideoFlags;
			}
		}
	}

	XStart = Xblanks;

	{
		CBruce *p = GSkater[0];
		int x = p->mFront.vx, y = p->mFront.vy, z = p->mFront.vz;

		p->mVel.vx = x * 20;
		p->mVel.vy = y * 20;
		p->mVel.vz = z * 20;
	}

	for (int i = 0; i < 10; i++)
		Ob_AI(&CameraList);

	GameFade = 0;
	GSkater[0]->mVel.vx = 0;
	GSkater[0]->mVel.vy = 0;
	GSkater[0]->mVel.vz = 0;

	while (!EndCode) {
		unsigned StartVblanks = Vblanks;

		PollHost();

		if (!GSkater[0]->mPhysicsState && (!DAT_800d17cc || !DAT_800d17cc->mPhysicsState) &&
		    GState == 1) {
			if (--GSpin <= 0) {
				if (GGame != 7 && GSkater[0]->mPanel.mLandedTotal) {
					GSpinSpeed = 0;
					GSpin = 1;
				} else {
					if (GSkater[0]->mGrindSnd) {
						SFX_Stop(GSkater[0]->mLoopSnd);

						GSkater[0]->mGrindSnd = 0;
					}

					if (GSkater[0]->mRollSnd) {
						SFX_Stop(GSkater[0]->mRollSnd);

						GSkater[0]->mRollSnd = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->mGrindSnd) {
						SFX_Stop(DAT_800d17cc->mGrindSnd);

						DAT_800d17cc->mGrindSnd = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->mRollSnd) {
						SFX_Stop(DAT_800d17cc->mRollSnd);

						DAT_800d17cc->mRollSnd = 0;
					}

					GState = 0;

					if (Levels[GLevel].isCompetition && GGame == 1)
						Front_SwitchOnEndHeat();
					else
						Front_SwitchOnEndLevelMenu();
				}
			}
		}

		Crowd_Update();
		CalcPolyBufferEnd();
		System_Logic();
		Video_HandleStepping();

		if (GState == 1 && !GamePaused) {
			if (!GSkater[0]->mPhysicsState) {
				uchar c = Pad[0].Grab.Pressed;

				Pad_ClearAllOne(0);

				Pad[0].Down.Pressed = 1;
				Pad[0].Grab.Pressed = c;
			}

			if (GNumberOfPlayers == 2 && !DAT_800d17cc->mPhysicsState) {
				uchar c = Pad[1].Grab.Pressed;

				Pad_ClearAllOne(1);

				Pad[1].Down.Pressed = 1;
				Pad[1].Grab.Pressed = c;
			}
		}

		if ((GNumberOfPlayers == 1 || (GGame == 7 && GHorseClearInput2)) && !Cheat_ScreenShots)
			Pad_ClearAllOne(1);

		Game_Logic();

		if (EndCode)
			break;

		Game_Logic();

		if (EndCode)
			break;

		Db_WaitRender();
		FPS_Display();
		::Display();

		if (!GameFrozen && Cheat_FPS)
			Panel_DrawFlatShadedPoly(448, 208, 54, 18, 0, 0, 0, 0);

		if (!DemoTimer)
			Front_PrintDemo();

		GameFMV_SyncFMV(
		    Levels[GLevel].FMV && (GNumberOfPlayers == 1 || GGame == 7) ? M3d_WireframeMode : 0);

		M3d_WireframeMode = 0;

		if (Vblanks == StartVblanks)
			Pause(1);

		Db_Render();

		if (Vblanks == StartVblanks + 1)
			Pause(1);

		if (GGame == 7) {
			if (!GHorseWordChosen) {
				GHorseDoneStart = 1;
				GHorseWordChosen = 1;
				Front_PrintHorseLetters();
			} else if (!GHorseDoneStart) {
				GHorseDoneStart = 1;
				Front_SwitchOnStartLevelMenu();
			}
		}

		if (MechList->IsDead())
			EndCode = 2;
	}
done:
	Spool_Sync();

	M3d_OTPushback = 1;
	Front_SaveGameState();
	Db_WaitRender();
	DrawSync(0);
	Db_Init();
	Crowd_Stop();
	Mess_SetCurrentFont(DAT_800d1798);
	Mess_UnloadFont();
	Mess_SetCurrentFont(GSkaterRegion);
	Mess_UnloadFont();
	Mess_SetCurrentFont(GStartTime);
	Mess_UnloadFont();
}

/* FUN_80026d44 (0x80026d44) */
int FUN_80026d44(void) {
	volatile SControl *p;

	PC_RunStaticCtors(); /* the game's static constructors (libsn's __main on the PS1) */
	Options_Init();
	Career_Init();
	FUN_80071f84();
	Init_Restart(PACK_0, 16);
	Db_Init();
	Pad_Update();

	p = Pad;

	if (p->Pause.Pressed == 0 && (p->SpinLeft.Pressed == 0 || p->TapSpinLeft.Pressed == 0))
		GameFMV_PlayMovie(-1);

	Pad_Update();

	p = Pad;

	if (p->Pause.Pressed == 0 && (p->SpinLeft.Pressed == 0 || p->TapSpinLeft.Pressed == 0))
		GameFMV_PlayMovie(-2);

	Pad_Update();

	p = Pad;

	if (p->Pause.Pressed == 0 && (p->SpinLeft.Pressed == 0 || p->TapSpinLeft.Pressed == 0))
		GameFMV_PlayMovie(-25);

	Init_Restart(PACK_0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	HiRes_Init(1);
	FUN_8006b3f8();
	return 0;
}

/* LaunchGame__Fv (0x80026e98) */
/* Fuzzy demo twin (0.30; THPS1 drops the attract-mode wait loop); locals and the
   DUN label from its MAIN.SYM. Re-initialises the packs keeping the save game across
   Init_ForGame, loads the game, then (unless a restart was requested) primes the pads and plays
   levels, looping back to DUN while the game mode says to carry on. */
int LaunchGame() {
	int Result = 0;

	Pack_Init();
	Init_Restart(PACK_0, 16);

	{
		SSaveGame tmp = StartupGameState;

		Init_ForGame();

		StartupGameState = tmp;
	}

	Front_SetPreferences(StartupGameState.Preferences);

	if (!Result) {
		LoadingScreen_Main();

		if (GAutoTest)
			DumpAutoTest();

		Front_LoadGame(&StartupGameState);
	}

	if (BackOut) {
		if (DemoTimer)
			StartScreen = 1;
		else
			StartScreen = 4;

		BackOut = 0;
		return 0;
	}

	if (GGame == 7)
		Horse_Reset();
DUN:
	Pad[0].Ollie.Pressed = 1;
	Pad[0].Ollie.Triggered = 0;
	Pad[1].Ollie.Pressed = 1;
	Pad[1].Ollie.Triggered = 0;
	CloseCam = 0;
	ViewportMode = 0;

	if (GNumberOfPlayers == 2 && GGame != 7)
		ViewportMode = 1;

	PlayAway();

	GClear1 = 0;
	GClear2 = 0;
	GHorseDoneStart = 0;

	if (EndCode == 7) {
		printf("ENDCODE_TWEAKJUMP\n");
		Init_Cleanup(1);
		Trig_ParseTRGFile();
		CreateSkatersAndCameras();
		Trig_ExecuteRestart();
		goto DUN;
	}

	printf(DAT_8009f5cc);
	Init_Restart(PACK_0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	HiRes_Init(1);
	return 0;
}
