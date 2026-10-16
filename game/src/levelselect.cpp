/* LEVELSELECT.CPP: THPS1's career level-select screen (no demo counterpart): level pictures
   streamed into pack slots, greyed out when locked (declarations in game/levelselect.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/levelselect.h"
#include "game/mem.h"
#include "pc_texstore.h"
#include "game/classsizes.h"
#include "game/m3d.h"
#include "game/shellutils.h"
#include "game/pre.h"
#include "game/career.h"
#include "game/pad.h"
#include "game/mess.h"
#include "game/fileio.h"
#include "game/sfx.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/psyq.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/pack.h"
#include "game/pal.h"

extern int DAT_800d2368;
extern int DAT_800d1dc4;
extern int DAT_800d2374;
extern int DAT_800d2380;
extern int DAT_800d2388;
extern int DAT_800d2378;
extern int DAT_800d238c;
extern int DAT_800d2390;
extern int DAT_800d237c;
extern int DAT_800d2384;
/* the level pictures' BMP files, as loaded */
extern char *DAT_800d3810[];
extern int DAT_800d3840[];
extern int DAT_800d2360, DAT_800d2364;
/* CClass::operator new and SlicedImage::SlicedImage, egcs-ABI calls (the C-source
   LevelSelect_DecodePics) */
extern "C" SlicedImage *__nw__6CClassUi(int);
extern "C" SlicedImage *__11SlicedImagePciiiiiUs(SlicedImage *, char *, int, int, int, int, int,
                                                  int);
extern int DAT_800d23b0[2];
extern int DAT_800d23b8[2];
extern int DAT_800d1dc8;
extern TSPack *DAT_800d23a8;
extern TSPack *DAT_800d23ac;
extern TSPack *DAT_800d2398[2];
extern TSPack *DAT_800d23a0[2];
/* unlocked (this visit) */
extern int DAT_800bf2a0[];
/* unlocked (saved) */
extern int DAT_800bf2c8[];
/* the level pictures */
extern SlicedImage *DAT_800bf2f0[];
/* skater */
extern int DAT_800d236c;
/* competition medals */
extern int DAT_800d2370;
extern char DAT_800a2310[];
/* egcs-ABI call, as retail makes it */
extern "C" void __dl__6CClassPv(void *p);

/* PC: the levels this visit's carousel shows, in order (the ones offered in the game mode: a mod's
   level names its modes, the game's own are in all of them); DAT_800d237c counts them. The
   carousel and its pictures go by position in this list. */
static int LevelSelect_Visible[MAX_LEVELS];

/* PC: the tapes that unlock a level: the career's count (the game's own levels'), and for a mod's
   level the tapes got on the mods' levels too */
static int LevelSelect_Tapes(int skater, int level) {
	int n = Career_Progress(skater)->NumTapes;

	if (level >= STOCK_LEVELS)
		for (int l = STOCK_LEVELS; l < MAX_LEVELS - 1 && *Levels[l].pLevelName; l++)
			if (!Levels[l].isCompetition)
				for (int k = 0; k < 5; k++)
					n += (*Career_Tapes(skater, l) >> k) & 1;

	return n;
}

/* LevelSelect_Run__Fi (0x8006658c) */
int LevelSelect_Run(int a) {
	DAT_800d2368 = a;
	Mess_SetCurrentFont("mainf.fnt");
	Mess_SetRGB(128, 128, 128, 0);
	Mess_SetFontScale(256);
	Mess_SetTextJustify(1);
	Mess_SetFontShadowOffset(4);
	LevelSelect_Screen();
	return DAT_800d1dc4;
}

/* MakeGreyClut__Fii (0x800665ec) */
/* Sibling of the demo's MakeTransparentClut2 (same locals): reads the 256-entry
   CLUT at (x, y) back from VRAM, turns every colour into an opaque grey of 55% of r+g+b, and
   uploads it again. */
void MakeGreyClut(int x, int y) {
	unsigned short *buf = (unsigned short *)Mem_NewX(512, 1, 1);
	RECT temp;
	unsigned short *buf2;

	temp.x = x;
	temp.y = y;
	temp.w = 256;
	temp.h = 1;

	PC_TexRead(temp.x, temp.y, temp.w, temp.h, buf);

	buf2 = buf;

	for (int i = 0; i < 256; i++) {
		unsigned int r = (*buf2 >> 10) & 31;
		unsigned int g = (*buf2 >> 5) & 31;
		unsigned int b = *buf2 & 31;
		unsigned int grey = (r + g + b) * 550 / 3000;

		*buf2 = ((grey << 10) + (grey << 5) + grey) | 0x8000;
		buf2++;
	}

	PC_TexUpload(temp.x, temp.y, temp.w, temp.h, buf);
	Mem_Delete(buf);
}

/* LevelSelect_StartPicLoad__Fi (0x800666f4) */
void LevelSelect_StartPicLoad(int a) {
	int i;
	int n = DAT_800d237c;

	DAT_800d2374 = -1;
	DAT_800d2380 = 0;
	DAT_800d2388 = 0;
	DAT_800d2378 = a;
	DAT_800d238c = 1;
	DAT_800d2390 = 0;

	for (i = 0; i < n; i++) {
		DAT_800d3840[i] = 0;
		DAT_800d3810[i] = 0;
		DAT_800bf2f0[i] = 0;
	}

	DAT_800d2384 = 0;
}

/* LevelSelect_ContinueLoading__Fv (0x80066768) */
/* SLevel (game/career.h) */
void LevelSelect_ContinueLoading(void) {
	char buf[64]; /* PC: 16 (mods' levels' pictures have longer names) */
	int n, t, one;

	FileIO_Continue();

	if (Loading == 0 && DAT_800d2380 == 0 && DAT_800d2390 < 4) {
		t = DAT_800d2374;

		if (t >= 0) {
			DAT_800d2390++;
			DAT_800d3840[DAT_800d2360] = one = 1;
		} else {
			DAT_800d2374 = DAT_800d2374 + 1;
			goto L50;
		}

		if (t < DAT_800d237c / 2)
			DAT_800d2374 = DAT_800d237c - t - 1;
		else {
			if (DAT_800d237c / 2 >= t) {
				DAT_800d2380 = one;
				return;
			}

			DAT_800d2374 = DAT_800d237c - t;
		}
	L50:
		DAT_800d2360 = DAT_800d2374 + DAT_800d2378;

		if (DAT_800d2360 >= DAT_800d237c)
			DAT_800d2360 -= DAT_800d237c;

		strcpy(buf, Levels[LevelSelect_Visible[DAT_800d2360]].pImageName);

		n = FileIO_Open(buf);

		if (n <= 0) {
			printf("\nFile %s not found", buf);
			return;
		}

		{
			char *r = Mem_NewX(n, 1, 1);

			DAT_800d3810[DAT_800d2360] = r;
			FileIO_Load(r);
		}
	}
}

/* LevelSelect_DecodePics__Fv (0x800668e8) */
void LevelSelect_DecodePics(void) {
	unsigned short a;
	int pa;
	char *b;
	int c, d;
	int s1, h, t;
	struct SlicedImage *v;
	struct SlicedImage **ps;

	if (DAT_800d2384 == 0)
		DAT_800d2364 = DAT_800d2378;

	s1 = DAT_800d238c;

	if (s1 == 1 && DAT_800d2388 == 0 && DAT_800d3840[DAT_800d2364]) {
		DAT_800d2390--;
		Load8BitBMP2(DAT_800d3810[DAT_800d2364], &b, &c, &d, &a);

		t = DAT_800d2364;
		DAT_800d3810[t] = 0;
		ps = &DAT_800bf2f0[t];
		v = __nw__6CClassUi(SIZEOF_SLICEDIMAGE);

		if (v)
			v = __11SlicedImagePciiiiiUs(v, b, c, d, 32, 63, 8, a);

		*ps = v;

		if (DAT_800bf2a0[LevelSelect_Visible[DAT_800d2364]] == 0)
			MakeGreyClut(Pal256X, Pal256Y + Get256SlotNumber(a));

		h = DAT_800d237c / 2;

		if (DAT_800d2384 < h)
			DAT_800d2384 = DAT_800d237c - DAT_800d2384 - 1;
		else {
			if (h >= DAT_800d2384) {
				DAT_800d2388 = s1;
				return;
			}

			DAT_800d2384 = DAT_800d237c - DAT_800d2384;
		}

		DAT_800d2364 = DAT_800d2378 + DAT_800d2384;

		if (DAT_800d2364 >= DAT_800d237c)
			DAT_800d2364 -= DAT_800d237c;
	}
}

/* LevelSelect_InitPackSlots__Fv (0x80066aa8) */
/* TSPack (game/m3d.h) */
void LevelSelect_InitPackSlots(void) {
	int i;
	TSPack *s;

	for (i = 0; i < 2; i++) {
		s = Pack_Pack(-1, 160, 256, &DAT_800d23b0[i], &DAT_800d23b8[i], 16, 0, 0);
		DAT_800d2398[i] = (TSPack *)Mem_NewX(sizeof(TSPack), 1, 1);
		*DAT_800d2398[i] = *s;
		DAT_800d2398[i]->pNext = 0;
		DAT_800d23a0[i] = 0;
	}

	DAT_800d23a8 = Pack_pFree;
	DAT_800d23ac = Pack_pFull;
}

/* LevelSelect_SelectPackSlot__Fi (0x80066bb4) */
/* Switches the current pair (Pack_pFree/64) to slot n: the outgoing pair is saved to the
   current slot (or to the default pair when the slot is -1), and the incoming one restored. */
void LevelSelect_SelectPackSlot(int n) {
	if (DAT_800d1dc8 == -1) {
		DAT_800d23a8 = Pack_pFree;
		DAT_800d23ac = Pack_pFull;
	} else {
		struct TSPack *a = Pack_pFree;
		struct TSPack *b = Pack_pFull;

		DAT_800d2398[DAT_800d1dc8] = a;
		DAT_800d23a0[DAT_800d1dc8] = b;
	}

	DAT_800d1dc8 = n;

	if (n == -1) {
		Pack_pFree = DAT_800d23a8;
		Pack_pFull = DAT_800d23ac;
	} else {
		Pack_pFree = DAT_800d2398[n];
		Pack_pFull = DAT_800d23a0[n];
	}
}

/* LevelSelect_FreePackSlots__Fv (0x80066c70) */
/* C++. Frees everything in both pack slots: selects the default slot, then for each slot
   unpacks its pack and walks its two allocation lists, freeing every node. */
void LevelSelect_FreePackSlots() {
	int i;

	LevelSelect_SelectPackSlot(-1);

	for (i = 0; i < 2; i++) {
		Pack_Unpack(DAT_800d23b0[i], DAT_800d23b8[i]);

		while (DAT_800d2398[i]) {
			TSPack *n = DAT_800d2398[i]->pNext;

			Mem_Delete(DAT_800d2398[i]);

			DAT_800d2398[i] = n;
		}

		while (DAT_800d23a0[i]) {
			TSPack *n = DAT_800d23a0[i]->pNext;

			Mem_Delete(DAT_800d23a0[i]);

			DAT_800d23a0[i] = n;
		}
	}
}

/* LevelSelect_Screen__Fv (0x80066d7c) */
/* C++ (front end, level select; called from LevelSelect_Run, which returns the chosen level or -1).
   Counts the levels in the level table, works out which are unlocked (career: tapes and
   competition medals of the current skater; other modes: the saved unlock flags, or the first
   five in the free-skate modes 5-8), then shows a map with an arrow over the selected level and a
   scrolling strip of level pictures (streamed in by LevelSelect_ContinueLoading/LevelSelect_DecodePics). Career shows the
   tape count, the three competition medals, the level's goal tapes / medal and "N tapes, N medals
   needed to unlock"; mode 3 shows the level's top three high scores. Left/right scroll (the level
   names slide out and back in), X/Start picks an unlocked level, triangle backs out. */
void LevelSelect_Screen() {
	int medal[3];

	FrontLoader_LoadSet(3);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("select_c.bmp"));
	Pack_DefragmentFully();
	LevelSelect_InitPackSlots();

	int numLevels;
	int *vis = LevelSelect_Visible;

	for (numLevels = 0; numLevels < MAX_LEVELS - 1; numLevels++)
		if (!*Levels[numLevels].pLevelName)
			break;

	DAT_800d237c = 0;

	for (int i = 0; i < numLevels; i++)
		if (Level_InMode(i, GGame))
			vis[DAT_800d237c++] = i;

	if (GGame == 5 || GGame == 6 || GGame == 8 || GGame == 7)
		DAT_800d2370 = 1;
	else
		DAT_800d2370 = 0;

	char *comps[3] = { "Skate Park", "Burnside", "Roswell" };
	int cur = 0;

	for (int i = 0; i < DAT_800d237c; i++)
		if (vis[i] == GCareer.LastLevel)
			cur = i;

	if (GGame == 1) {
		for (int i = 0; i < numLevels; i++)
			DAT_800bf2a0[i] = 0;
	} else {
		for (int i = 0; i < numLevels; i++)
			DAT_800bf2a0[i] = DAT_800bf2c8[i];
	}

	int first;
	int last;

	if (GGame == 1) {
		first = DAT_800d2368;
		last = first + 1;
	} else {
		first = 0;
		last = GNumSkaters; /* PC: 11 */
	}

	for (int s = first; s < last; s++) {
		DAT_800d236c = 0;

		for (int k = 0; k < 3; k++) {
			int j = 0;

			while (strcmp(Levels[j].pLevelName, comps[k]))
				j++;

			uchar f = *Career_Tapes(s, j);

			if (f & 0x20) {
				medal[k] = 0;
				DAT_800d236c++;
				Career_Progress(s)->NumGold++;
			} else if (f & 0x40) {
				medal[k] = 1;
				DAT_800d236c++;
				Career_Progress(s)->NumSilver++;
			} else if (f & 0x80) {
				medal[k] = 2;
				DAT_800d236c++;
				Career_Progress(s)->NumBronze++;
			} else {
				medal[k] = -1;
			}
		}

		for (int i = 0; i < numLevels; i++) {
			if ((LevelSelect_Tapes(s, i) >= Levels[i].TapesToUnlock &&
			     DAT_800d236c >= Levels[i].MedalsToUnlock) ||
			    (DAT_800d2370 && i < 5)) {
				DAT_800bf2a0[i] = 1;
				DAT_800bf2c8[i] = 1;
			}
		}
	}

	int ax = Levels[vis[cur]].map_loc[0] - 16;
	int ay = Levels[vis[cur]].map_loc[1] - 16;
	int arrowTimer[2];
	int arrowX[2];
	int arrowY[2];
	int unused60[2]; /* unused; keeps the frame layout */
	Sprite *arrowSpr[4];
	char name[16];
	Sprite *arrow = new Sprite("levarrow.bmp", 1, 32, 128);
	EraseSprite *save = new EraseSprite(ax, ay, 32, 32);

	arrowTimer[0] = 0;
	arrowTimer[1] = 0;
	arrowX[0] = 91;
	arrowY[0] = 158;
	arrowX[1] = 583;
	arrowY[1] = 158;

	for (int i = 0; i < 4; i++) {
		sprintf(name, "levbut%d.bmp", i + 1);

		arrowSpr[i] = new Sprite(name, 1, 32, 128);
	}

	Sprite *tapeBox = new Sprite("tape_s3.bmp", 1, 32, 128);
	Sprite *check = new Sprite("check1.bmp", 1, 32, 128);
	Sprite *frame = new Sprite(266, 93, 180, 34, 1, 32, 128);
	int state = 0;
	int timer;

	Mess_LoadFont("goalf.fnt", -1, -1, -1);

	char line1[32];
	char line2[32];
	int lineX[2];
	int lineY[2];

	if (GGame == 1) {
		strcpy(line2, "needed to unlock");
	} else {
		strcpy(line1, "play career mode");
		strcpy(line2, "to unlock");
	}

	lineX[1] = 357 - (Mess_TextWidth(line2) >> 1);
	lineY[1] = Mess_TextHeight(line2) + 198 - Mess_TextHeightBelowBaseline(line2);
	Sprite *medalSpr[3];
	Sprite *goalSpr[3];

	for (int i = 0; i < 3; i++) {
		sprintf(name, "medal%d.bmp", i + 1);

		medalSpr[i] = new Sprite(name, 1, 32, 128);
		sprintf(name, "medal%d.bmp", i + 4);

		goalSpr[i] = new Sprite(name, 1, 32, 128);
	}

	Sprite *itemBox = new Sprite("itembox2.bmp", 1, 32, 128);

	Mess_LoadFont("tapes1.fnt", -1, -1, -1);
	Mess_SetFontScale(255);

	char tapes[3];
	uchar n = Career_Progress(DAT_800d2368)->NumTapes;

	tapes[2] = 0;
	tapes[0] = n / 10 + '0';
	tapes[1] = n % 10 + '0';
	int tapesY = (Mess_TextHeight(tapes) >> 1) + 359 - Mess_TextHeightBelowBaseline(tapes);

	if (GGame == 1) {
		for (int i = 0; i < 2; i++) {
			int y = 307;

			HiRes_ClearOTag();
			ResetOT();
			itemBox->draw(375, 287, 0, -1, -1, -1);
			Mess_DrawText(472, tapesY, tapes, 0, 0, 256, 0);

			for (int k = 0; k < 3; k++) {
				if (medal[k] != -1)
					medalSpr[medal[k]]->draw(575, y, 0, -1, -1, -1);

				y += 24;
			}

			HiRes_DrawOTag();
		}
	}

	delete itemBox;
	Sprite *scoreBox = new Sprite("scorebox.bmp", 1, 32, 128);

	for (int i = 0; i < 2; i++) {
		HiRes_ClearOTag();
		ResetOT();

		if (GGame == 3)
			scoreBox->draw(375, 287, 0, -1, -1, -1);

		HiRes_DrawOTag();
	}

	delete scoreBox;
	Sprite *scoreBack = new Sprite(409, 320, 167, 61, 1, 32, 128);

	Mess_LoadFont("stats1.fnt", -1, -1, -1);

	Sprite *topBar = new Sprite(0, 55, 640, 40, 1, 32, 128);

	char text[2][32];
	int tx[2];
	int ty[2];
	int tw[2];
	int th[2];
	int ttarget[2];
	int tstep[2];

	strcpy(text[0], Levels[vis[cur]].pLevelName);
	strcpy(text[1], Levels[vis[cur]].pCityName);
	Mess_SetCurrentFont("mainf.fnt");

	tw[0] = Mess_TextWidth(text[0]);
	tw[1] = Mess_TextWidth(text[1]);
	int mid = (tw[0] + tw[1]) / 2 + 320 - tw[1];

	th[0] = Mess_TextHeight(text[0]);
	int w20 = tw[0] + 20;

	tx[0] = mid - w20;
	ty[0] = th[0] / 2 + 71 - Mess_TextHeightBelowBaseline(text[0]);
	ttarget[0] = -tw[0];
	tstep[0] = (ttarget[0] - tx[0]) / 7;
	th[1] = Mess_TextHeight(text[1]);
	tx[1] = mid + 20;
	ty[1] = th[1] / 2 + 71 - Mess_TextHeightBelowBaseline(text[1]);
	ttarget[1] = 640;
	tstep[1] = (ttarget[1] - tx[1]) / 7;

	LevelSelect_SelectPackSlot(0);

	int picX[2];
	SlicedImage *pic[2];
	int picIdx[2];

	picX[0] = 135;
	picX[1] = 583;
	pic[0] = 0;
	pic[1] = 0;
	picIdx[0] = cur;
	picIdx[1] = cur + 1;

	if (picIdx[1] >= DAT_800d237c)
		picIdx[1] = 0;

	int speed = 0;
	int anim = 0;

	LevelSelect_StartPicLoad(picIdx[0]);

	int sel = picIdx[0];
	int next = sel + 2;

	if (next >= DAT_800d237c)
		next -= DAT_800d237c;

	int prev = sel - 1;

	if (sel == 0)
		prev = DAT_800d237c - 1;

	char unused188[64]; /* unused; keeps the frame layout */

	PadBuffer input(-1);

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	while (1) {
		Front_UpdateMusicLoop();

		if (!DAT_800d2380)
			LevelSelect_ContinueLoading();

		LevelSelect_DecodePics();
		HiRes_ClearOTag();
		ResetOT();
		frame->draw(266, 93, 0, -1, -1, -1);
		SetClippingBox(137, 115, 442, 154);

		for (int i = 0; i < 2; i++) {
			if (pic[i])
				pic[i]->draw(picX[i], 112, 0, -1, -1, -1, 0, 0);
			else
				DrawColoredBox(picX[i], 112, 448, 161, 0, 0, 0);
		}

		SetClippingBox(0, 0, 640, 480);
		arrow->draw(ax, ay, 0, -1, -1, -1);
		topBar->draw(0, 55, 0, -1, -1, -1);

		if (GGame == 1 && state == 0) {
			if (!Levels[vis[sel]].isCompetition) {
				int x = 266;
				uchar f = *Career_Tapes(DAT_800d2368, vis[sel]);

				for (int k = 0; k < 5; k++) {
					tapeBox->draw(x, 93, 0, -1, -1, -1);

					if ((f >> k) & 1)
						check->draw(x, 93, 0, -1, -1, -1);

					x += 36;
				}
			} else {
				uchar f = *Career_Tapes(DAT_800d2368, vis[sel]);

				if (f & 0x20)
					goalSpr[0]->draw(335, 88, 0, -1, -1, -1);
				else if (f & 0x40)
					goalSpr[1]->draw(335, 88, 0, -1, -1, -1);
				else if (f & 0x80)
					goalSpr[2]->draw(335, 88, 0, -1, -1, -1);
			}
		}

		if (GGame == 3 && state == 0) {
			char score[16];

			scoreBack->draw(409, 320, 0, -1, -1, -1);
			Mess_SetCurrentFont("stats1.fnt");
			int y = (Mess_TextHeight("0") >> 1) + 333 - Mess_TextHeightBelowBaseline("0");

			for (int k = 0; k < 3; k++) {
				Mess_SetTextJustify(1);
				Mess_DrawText(409, y, Career_HighScores(vis[sel])[k].Name, 0, 0, 256, 0);
				sprintf(score, "%d", Career_HighScores(vis[sel])[k].Score);
				Mess_SetTextJustify(2);
				Mess_DrawText(578, y, score, 0, 0, 256, 0);

				y += 20;
			}

			Mess_SetTextJustify(1);
		}

		if (state == 0) {
			char plural[4];

			plural[0] = 0;
			plural[1] = 0;
			plural[2] = 0;
			plural[3] = 0;
			Mess_SetCurrentFont("goalf.fnt");

			if (GGame == 1) {
				if (LevelSelect_Tapes(DAT_800d2368, vis[sel]) < Levels[vis[sel]].TapesToUnlock) {
					if (DAT_800d236c < Levels[vis[sel]].MedalsToUnlock) {
						if (Levels[vis[sel]].TapesToUnlock >= 2)
							plural[0] = 's';

						if (Levels[vis[sel]].MedalsToUnlock >= 2)
							plural[2] = 's';

						sprintf(line1, "%d tape%s, %d medal%s", Levels[vis[sel]].TapesToUnlock,
						        &plural[0], Levels[vis[sel]].MedalsToUnlock, &plural[2]);
					} else {
						if (Levels[vis[sel]].TapesToUnlock >= 2)
							plural[0] = 's';

						sprintf(line1, "%d tape%s", Levels[vis[sel]].TapesToUnlock, plural);
					}
				} else if (DAT_800d236c < Levels[vis[sel]].MedalsToUnlock) {
					if (Levels[vis[sel]].MedalsToUnlock >= 2)
						plural[0] = 's';

					sprintf(line1, "%d medal%s", Levels[vis[sel]].MedalsToUnlock, plural);
				}
			}

			if (!DAT_800bf2a0[vis[sel]]) {
				lineX[0] = 357 - (Mess_TextWidth(line1) >> 1);
				lineY[0] = 190 - Mess_TextHeightBelowBaseline(line1);
				Mess_DrawText(lineX[0], lineY[0], line1, 1, 0, 256, 0);
				Mess_DrawText(lineX[1], lineY[1], line2, 1, 0, 256, 0);
			}
		}

		Mess_SetCurrentFont("mainf.fnt");

		for (int i = 0; i < 2; i++)
			Mess_DrawText(tx[i], ty[i], text[i], 0, 0, 256, 0);

		for (int i = 0; i < 2; i++) {
			int f = 0;

			if (arrowTimer[i]) {
				f = 2;
				arrowTimer[i]--;
			}

			arrowSpr[i + f]->draw(arrowX[i], arrowY[i], 0, -1, -1, -1);
		}

		HiRes_DrawOTag();

		for (int i = 0; i < 2; i++) {
			SlicedImage *im = DAT_800bf2f0[picIdx[i]];

			if (im && !pic[i]) {
				pic[i] = im;
				LevelSelect_SelectPackSlot(i);
				pic[i]->pack();
			}

			picX[i] += speed;

			if (picX[i] < -313 && speed < 0) {
				picX[i] += 896;
				LevelSelect_SelectPackSlot(i);

				if (pic[i])
					pic[i]->unpack();

				pic[i] = DAT_800bf2f0[next];
				picIdx[i] = next;

				if (pic[i])
					pic[i]->pack();

				if (++prev >= DAT_800d237c)
					prev = 0;

				if (++next >= DAT_800d237c)
					next = 0;
			} else if (picX[i] >= 583 && speed > 0) {
				picX[i] -= 896;
				LevelSelect_SelectPackSlot(i);

				if (pic[i])
					pic[i]->unpack();

				pic[i] = DAT_800bf2f0[prev];
				picIdx[i] = prev;

				if (pic[i])
					pic[i]->pack();

				if (--next < 0)
					next = DAT_800d237c - 1;

				if (--prev < 0)
					prev = DAT_800d237c - 1;
			}
		}

		if (state == 1) {
			if (--timer == 0)
				state = 0;
		}

		if (anim > 0) {
			tx[0] += tstep[0];
			tx[1] += tstep[1];

			if (anim == 8) {
				if (!strcmp(Levels[vis[sel]].pLevelName, "Roswell") && !DAT_800bf2a0[vis[sel]]) {
					strcpy(text[0], "unknown");
					strcpy(text[1], "unknown");
				} else {
					strcpy(text[0], Levels[vis[sel]].pLevelName);
					strcpy(text[1], Levels[vis[sel]].pCityName);
				}

				Mess_SetCurrentFont("mainf.fnt");

				tw[0] = Mess_TextWidth(text[0]);
				tw[1] = Mess_TextWidth(text[1]);
				mid = (tw[0] + tw[1]) / 2 + 320 - tw[1];
				th[0] = Mess_TextHeight(text[0]);
				tx[0] = -tw[0];
				ty[0] = th[0] / 2 + 71 - Mess_TextHeightBelowBaseline(text[0]);
				int w20b = tw[0] + 20;

				ttarget[0] = mid - w20b;
				tstep[0] = (ttarget[0] - tx[0]) / 7;
				tw[1] = Mess_TextWidth(text[1]);
				th[1] = Mess_TextHeight(text[1]);
				tx[1] = 640;
				ty[1] = th[1] / 2 + 71 - Mess_TextHeightBelowBaseline(text[1]);
				ttarget[1] = mid + 20;
				tstep[1] = (ttarget[1] - tx[1]) / 7;
			}

			if (anim == 1) {
				DAT_800d238c = 4;
				tstep[0] = -tstep[0];
				tstep[1] = -tstep[1];
			}

			if (--anim == 0) {
				state = 1;
				timer = 4;
				speed = 0;
			}
		}

		if (anim == 0 && DAT_800d238c < 2) {
			int press = input.getPress();

			if (press == 1) {
				speed = 32;
				anim = 14;
				sel--;
				DAT_800d238c = 0;

				if (sel < 0)
					sel = DAT_800d237c - 1;

				arrowTimer[0] = anim;
			} else if (press == 2) {
				do {
					speed = -32;
					int a = 14;

					anim = a;
					sel++;
					DAT_800d238c = 0;

					if (sel >= DAT_800d237c)
						sel = 0;

					arrowTimer[1] = a;
				} while (0);
			}

			if (press == 1 || press == 2) {
				SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);
				save->restore(ax, ay);

				ax = Levels[vis[sel]].map_loc[0] - 16;
				ay = Levels[vis[sel]].map_loc[1] - 16;
				DrawSync(0);
				LevelSelect_SelectPackSlot(-1);
				delete save;
				save = new EraseSprite(ax, ay, 32, 32);
				state = 2;
			}
		}

		if (DAT_800d238c >= 2)
			DAT_800d238c--;

		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			DAT_800d1dc4 = vis[sel];

			if (DAT_800bf2a0[vis[sel]]) {
				SFX_PlayX(SND_SELECT1, 0x2000, 0);

				GCareer.LastLevel = vis[sel];
				break;
			}

			SFX_PlayX(SND_NOSELECT1, 0x2000, 0);

			if (Cheat_LevelSelect) {
				GCareer.LastLevel = vis[sel];
				break;
			}
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(SND_GOBACK2, 0x2000, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			DAT_800d1dc4 = -1;
			break;
		}

		input.update();
	}

	if (Loading)
		FileIO_Sync();

	HiRes_BlankScreen();

	for (int i = 0; i < DAT_800d237c; i++) {
		delete DAT_800bf2f0[i];

		if (DAT_800d3810[i])
			Mem_Delete(DAT_800d3810[i]);
	}

	LevelSelect_FreePackSlots();
	Mess_SetCurrentFont("goalf.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("tapes1.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("stats1.fnt");
	Mess_UnloadFont();
	SpoolerBlocksFileIO->unloadPRE("Level.pre");

	delete scoreBack;
	delete tapeBox;
	delete check;

	for (int i = 0; i < 3; i++) {
		delete medalSpr[i];
		delete goalSpr[i];
	}

	delete frame;

	for (int i = 0; i < 4; i++)
		delete arrowSpr[i];

	delete save;
	delete arrow;
	delete topBar;
}

/* FUN_80068aac (0x80068aac) */
void FUN_80068aac(char *p, int f) {
	*(char **)(p + 0) = DAT_800a2310;

	if (f & 1)
		__dl__6CClassPv(p);
}
