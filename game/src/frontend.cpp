/* FRONTEND (THPS1's front-end screens, 0x80050c5c-0x80061714; no demo counterpart): board and
   skater select (1P/2P), the front-end music loop, the FileLoader wrappers (FrontLoader_*), the
   loading screens, the generic vertical-menu loop and the options/memory-card/title screens
   (declarations in game/frontend.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/frontend.h"
#include "game/shellutils.h"
#include "game/career.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/psyq.h"
#include "pc_frame.h"
#include "game/pre.h"
#include "game/pad.h"
#include "game/mess.h"
#include "game/sfx.h"
#include "game/hires.h"
#include "game/main.h"
#include "game/spool.h"
#include "game/bruce.h"
#include "game/m3dasm.h"
#include <libgpu.h>
#include "game/dbuffer.h"
#include "game/pack.h"
#include "game/front.h"
#include "game/m3d.h"
#include "game/ob.h"
#include "pc_draw.h"
#include "game/class.h"
#include "game/video.h"
#include "game/gamefmv.h"
#include "game/card.h"
#include "game/credits.h"
#include "game/db.h"
#include "game/levelselect.h"
#include "pc_screen.h"
#include "game/classsizes.h"
#include "game/padbuttons.h"
#include <stdlib.h>
#include "pc_input.h"
#include "pc_video.h"
#include "game/vector.h"
#include "game/rfront.h"
#include "game/camera.h"
#include "game/utils.h"
#include "game/bit.h"
#include "game/competition.h"
#include "game/globals.h"
#include "game/init.h"
#include "game/shell.h"
#include "pc_runtime.h"

/* the PS1 (egcs) vtable of a CClass object as the C sources walk it: entry 0 unused, then
   {this offset, function} pairs; slot 1 the (deleting) destructor, slot 2 VerticalMenu::draw */
struct SPsxVtable {
	short off0, i0;
	void *fn0;
	short off;
	short pad;
	void (*fn)(char *, int);
	short off2;
	short pad2;
	void (*fn2)(char *);
};

/* a CClass object seen through its vptr (at +0) */
struct SPsxVtObject {
	SPsxVtable *vt;
};
#define PSX_VT(p) (((SPsxVtObject *)(p))->vt)

/* the front-end menus' label and "disabled" tables (copied whole into locals) */
struct SMenuText3 {
	char *s[3];
};
struct SMenuText5 {
	char *s[5];
};
struct SMenuText6 {
	char *s[6];
};
struct SMenuText7 {
	char *s[7];
};
struct SMenuFlags3 {
	int v[3];
};
struct SMenuFlags5 {
	int v[5];
};
struct SMenuFlags7 {
	int v[7];
};

/* the title screen's two copyright lines */
struct C30 {
	char c[30];
};
struct C28 {
	char c[28];
};

/* The C sources' own views of a few globals, for the functions whose code depends on them
   (`using` in the function): gcc treats an array of unknown size, or a trailing one-element array,
   differently from the declared types (it may not load ahead of the test, it indexes from the
   object's start). Same symbols as the declarations above. */
/* Front_GetPadFunctionName: GameText as an array of strings; the four button functions' names
   (pGrindSlide..pJumpOllie) at 55 */
namespace PadFunctionNameView {
	extern "C" char *GameText[];
}
struct SPadFunctionNames {
	char *GrindSlide, *GrabTrick, *FlipTrick, *FlipTrickKick, *JumpOllie;
};
/* Front_StorePadMapping: GPreferences.ControllerFunction as a trailing array of mappings, and the
   edited mapping as an unsized array */
struct SPadMapFields {
	int Function0, Function1, Function2, Function3;
};
struct SPrefsPadMaps {
	char pad[0x120];
	SPadMapFields ControllerFunction[1];
};
namespace StorePadMappingView {
	extern "C" SPrefsPadMaps GPreferences;
	extern "C" SPadMapFields DAT_800bf260[];
}
/* the same Pad with its size, as the soft-reset check, the loading screens, the controller and
   sound options and the memory-card screen saw it (`using SizedPad::Pad;`): knowing both pads
   are there lets gcc test them together, which the other functions' unsized view doesn't */
namespace SizedPad {
	extern "C" SControl Pad[2];
}
extern int DAT_800d2c1c[3];
/* pictures per column */
extern char DAT_800d2c2c[2][3][15][16];
/* picture file names */
extern SlicedImage *DAT_800d31cc[2][3][15];
extern int DAT_800d333c[2][3];
/* load offset from the start entry (-1: not started) */
extern int DAT_800d335c[2][3];
/* start entry */
extern int DAT_800d337c[2][3];
/* column fully loaded */
extern int DAT_800d22ac;
/* player being loaded */
extern SlicedImage *DAT_800d22b0;
/* "locked" picture */
extern int DAT_800d22b8[2];
/* column being loaded, per player */
extern char *DAT_800d22c0[2];
/* load buffer, per player */
extern uchar DAT_800d22c8;
extern int DAT_800d22cc;
/* all pictures loaded */
extern int DAT_800d22d0[2];
/* column fully loaded */
extern int DAT_800d22a8;
extern int DAT_800be42c[];
/* per-player screen layout (player 0, player 1) */
extern int DAT_800d1bac[2];
/* text bar x */
extern int DAT_800d1bb4[2];
/* text bar width */
extern int DAT_800d1bbc[2];
/* text slide target x */
extern int DAT_800d1bc4[2];
/* stat rows x */
extern int DAT_800d1bcc[2];
/* deck column x */
extern int DAT_800d1bd4[2];
extern int DAT_800d1bdc[2];
/* trucks column x */
extern int DAT_800d1be4[2];
/* "board chosen" x */
extern int DAT_800d1bec[2];
/* wheels column x */
extern int DAT_800d1bf4[2];
/* word-aligned label buffers (copied with lw/lh/lb) */
typedef char SLabel6[6] __attribute__((aligned(4)));
typedef char SLabel7[7] __attribute__((aligned(4)));
extern int DAT_800d3394[3];
/* load offset from the start entry (-1: not started) */
extern int DAT_800d33a4[3];
/* start entry */
extern int DAT_800d33b4[3];
/* column fully loaded */
extern int DAT_800d33c4[3];
/* pictures per column */
extern char DAT_800d33d4[3][15][16];
/* picture file names */
extern SlicedImage *DAT_800d36a4[3][15];
extern SlicedImage *DAT_800d22dc;
/* "locked" picture */
extern int DAT_800d22e0;
extern char *DAT_800d22e4;
extern uchar DAT_800d22e8;
extern int DAT_800d22ec;
extern int DAT_800d22f0;
extern int DAT_800d22d8;
extern int DAT_800be438[];
extern int DAT_800d3788[MAX_SKATERS];
extern int DAT_800d231c[2];
extern int DAT_800d230c;
extern int DAT_800d2310;
extern int DAT_800d2314;
extern int DAT_800d3758[];
extern int DAT_800d22f4;
extern int DAT_800d2304[2];
extern char DAT_800be444[];
extern int DAT_800d2300;
extern int DAT_800d22fc;
extern int DAT_800d22f8;
extern int DAT_800d1c1c;
extern int DAT_800d1c20;
extern int DAT_800d1c24;
extern int DAT_800d1c18;
extern int DAT_800d1c10;
extern int DAT_800d1c0c;
extern int DAT_800d1c44;
int FUN_8008cf04(void);
extern int DAT_800d2338;
extern int DAT_800d2340;
extern int DAT_800d2344;
extern int DAT_800d233c;
extern int DAT_800d37e4[];
extern int DAT_800d37b4[];
extern int DAT_800d2324;
extern int DAT_800d1c40;
extern int DAT_800d232c;
extern int DAT_800d2328;
/* the skater-select pack slots: the skater's own (DAT_800d1c84/88 free/full), the shell's
   (DAT_800d1c8c/90) and the skater area's first slot (DAT_800d1c80) */
extern TSPack *DAT_800d1c84;
extern TSPack *DAT_800d1c80;
extern TSPack *DAT_800d1c88;
extern TSPack *DAT_800d1c8c;
extern TSPack *DAT_800d1c90;
/* the skater area's VRAM position (Pack_Pack's X and Y) */
extern int DAT_800d1c94;
extern int DAT_800d1c98;
extern int DAT_800d1c5c;
/* emit this TU's out-of-line copy of CClass's inline destructor and its vtable (retail
   0x8005bb3c / 0x800a18e4, after Front_CalcSkaterPreviewBounds), as retail's FRONT end TU did */
#pragma implementation "class.h"
extern int DAT_800d2330;
extern int DAT_800d2334;
extern int DAT_800d1c48;
extern int DAT_800d1c4c;
extern int DAT_800d1c50;
extern int DAT_800d1c54;
extern int DAT_800d1c58;
extern int DAT_800d1c60;
/* secret skater unlocked */
extern int DAT_800d1c64;
enum EPackInitType { PACK_0, PACK_1 };
struct SName {
	char c[10];
};
extern int DAT_800d1d80;
extern int DAT_800d1dc0;
/* looping sound */
extern short DAT_800d1ca0;
extern int DAT_800d1cb4;
extern int DAT_800d2348;
/* quit countdown */
extern unsigned DAT_800d1ca4;
extern int DAT_800d1cbc;
extern int DAT_800d1cc0;
extern int DAT_800d1cc4;
extern int DAT_800d1ca8;
extern int DAT_800d1cac;
extern SScreenFiles DAT_800be478[15];
/* the front end's FileLoader */
extern FileLoader DAT_800bec34;
/* FileLoader's members written in C (shellutils) */
extern "C" void FUN_80065ca4(FileLoader *);
extern "C" int FUN_80065fd4(FileLoader *);
extern "C" void FUN_80065d14(FileLoader *);
extern "C" void FUN_80065d80(FileLoader *);
extern "C" char *FUN_80065f1c(FileLoader *, char *name);
/* level, screen type and skater of the loading screen */
extern int DAT_800d1cf4;
extern unsigned DAT_800d1cf8;
extern int DAT_800d234c;
extern "C" DRAWENV *FUN_80092e70(DRAWENV *, int, int, int, int);
extern "C" void FUN_8005cd14(void);
extern "C" void FUN_8005d00c(void);
extern "C" void FUN_8005d28c(void);
extern int DAT_800d1cec, DAT_800d1d14;
extern Sprite *DAT_800d1d04;
extern Sprite *DAT_800d1d0c;
extern Sprite *DAT_800d1d00;
extern "C" void FUN_80061a10(char *);
extern "C" void FUN_8005dc58(int);
/* CClass::operator new by its egcs name: a plain call (the C sources' new; g++'s operator new
   would be malloc-like for the optimizer) */
extern "C" void *__nw__6CClassUi(unsigned);
/* constructors called by their egcs names (they return `this`) */
extern "C" Sprite *FUN_80063a5c(void *, int, char *, int, int, int, int);
extern "C" Sprite *__6SpritePciii(void *, char *, int, int, int);
extern "C" Sprite *__6Spriteiiiiiii(void *, int, int, int, int, int, int, int);
extern "C" PadBuffer *__9PadBufferi(void *, int);
extern "C" VerticalMenu *__12VerticalMenuiiiPPciPiT6i(void *, int, int, int, char **, int, int *,
                                                       int *, int);
extern uchar DAT_800d1d18;
void FUN_8005da40(void);
extern int DAT_800bf0d4[3];
/* picture x by screen */
extern int DAT_800bf0e0[3];
/* START+SELECT quit */
extern int DAT_800d1ce8;
/* "Press X" */
extern int DAT_800d1d08;
/* the menu input of the loading screen (any pad) */
extern PadBuffer DAT_800bf104;
extern int DAT_800bf0dc[], DAT_800bf0e8[];
extern int DAT_800d1ce4;
extern unsigned DAT_800d1cdc;
extern unsigned DAT_800d1ce0;
extern int DAT_800d1cd8;
void FUN_8005d370(void);
/* the countdown bar's x and y by screen */
extern int DAT_800bf0ec[], DAT_800bf0f0[], DAT_800bf0f4[];
extern int DAT_800bf0f8[], DAT_800bf0fc[], DAT_800bf100[];
/* CClass's vtable (the vptr ~CClass stores) */
extern char DAT_800a1b94[];
/* CClass::operator delete, called by its egcs name from the deleting destructor below */
extern "C" void __dl__6CClassPv(void *);
/* FUN_8005de08 (0x8005de08) */
/* C++ (the menu-input module's static constructor, _GLOBAL_.I): copies two words into
   DAT_800d1cc8/DAT_800d1ccc, then builds DAT_800bf104, the PadBuffer for any pad (-1) the
   front end reads (FUN_8005ddec is its teardown). */
int DAT_800d1cc8 = DAT_800bf0f4[0];
int DAT_800d1ccc = DAT_800bf100[0];
PadBuffer DAT_800bf104(-1);
extern Sprite *DAT_800d2358, *DAT_800d235c;
extern int FUN_8005e754(int);
extern int DAT_800d1d28;
int FUN_8005e09c(int *);
extern SMenuText6 DAT_800a1c44;
extern int DAT_800d1d2c;
int FUN_8005e8e0(int);
int FUN_8005ea00(int);
int FUN_80060274(int);
int FUN_800604c8(int);
int FUN_80060854(int);
void FUN_8005e728(VerticalMenu *);
/* PC: QUIT after VIEW VIDEOS. Its id 7 is shared with the hidden cheat menu, which is only
   reached through the menu path in a (FUN_8005e09c), so a 7 picked by hand means quit. */
extern "C" {
	static char s_quit[] = "QUIT";
}
extern SMenuText3 DAT_800a1c84;
int FUN_8005e4ec(char **, int, int);
extern SMenuText3 DAT_800a1cac;
/* the controller remap: the edited mapping and the mapping before the last change */
extern SPadMap DAT_800bf260[2];
extern SPadMap DAT_800bf280[2];
/* row last changed with left/right */
extern uchar DAT_800d2350;
extern SLabels DAT_800a1d0c;
enum { ROW_DEVICE, ROW_AUTOKICK, ROW_VIBRATION, ROW_HEADER, ROW_ACTION, ROW_RESET, ROW_BACK };
struct ControlsRow {
	uchar kind;
	uchar action; /* PcAction for ROW_ACTION */
	const char *text;
};
static const ControlsRow s_rows[] = {
	{ ROW_DEVICE, 0, 0 },
	{ ROW_AUTOKICK, 0, 0 },
	{ ROW_VIBRATION, 0, 0 },
	{ ROW_HEADER, 0, "SKATING" },
	{ ROW_ACTION, PC_ACT_UP, 0 },
	{ ROW_ACTION, PC_ACT_DOWN, 0 },
	{ ROW_ACTION, PC_ACT_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_OLLIE, 0 },
	{ ROW_ACTION, PC_ACT_FLIP, 0 },
	{ ROW_ACTION, PC_ACT_GRAB, 0 },
	{ ROW_ACTION, PC_ACT_GRIND, 0 },
	{ ROW_ACTION, PC_ACT_SPIN_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_SPIN_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_TAP_SPIN_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_TAP_SPIN_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_CAMERA, 0 },
	{ ROW_ACTION, PC_ACT_PAUSE, 0 },
	{ ROW_HEADER, 0, "MENUS" },
	{ ROW_ACTION, PC_ACT_MENU_UP, 0 },
	{ ROW_ACTION, PC_ACT_MENU_DOWN, 0 },
	{ ROW_ACTION, PC_ACT_MENU_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_ACCEPT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_BACK, 0 },
	{ ROW_RESET, 0, "RESET TO DEFAULT" },
	{ ROW_BACK, 0, "BACK TO OPTIONS" },
};
#define NUM_ROWS       ((int)(sizeof s_rows / sizeof s_rows[0]))
#define VISIBLE_ROWS   13
#define ROW_TOP        70 /* centre of the first visible row */
#define ROW_HEIGHT     24
#define NAME_X         130 /* action names, left-justified */
#define SLOT_X0        400 /* the two binding columns, centred */
#define SLOT_X1        535
#define HINT_Y         406
#define CAPTURE_FRAMES 300
/* (this font has letters and digits but no '.', '-' or ':', so the screen's own text is words) */
/* the area redrawn every frame (640x480 front-end coordinates = VRAM), and the background there */
static RECT s_area = { 110, 40, 520, 380 };
static ulong *s_saved;
enum {
	GFX_MODE,
	GFX_RESOLUTION,
	GFX_WIDESCREEN,
	GFX_DRAW_DISTANCE,
	GFX_OBJECT_DISTANCE,
	GFX_FOG,
	GFX_SHADOWS,
	GFX_ANTIALIASING,
	GFX_APPLY,
	GFX_BACK,
	GFX_ROWS
};
#define KEEP_FRAMES (15 * 60)
static const char *const s_modeLabels[PC_VIDEO_MODES] = { "WINDOWED", "BORDERLESS", "FULLSCREEN" };
static const char *const s_fogLabels[PC_FOG_MODES] = { "NORMAL", "LIGHT", "OFF" };
static const char *const s_shadowLabels[PC_SHADOWS_MODES] = { "PLAYSTATION", "DREAMCAST" };
/* the distance steps, in percent (Settings.ini may hold others in between); the last, the entire
   level, only for the draw distance */
static const int s_distances[] = { 100, 200, 300, 400, PC_DISTANCE_FULL };
#define DRAW_STEPS   5
#define OBJECT_STEPS 4
extern Sprite *DAT_800d1d5c;
extern Sprite *DAT_800d1d60;
extern SMenuText3 DAT_800a1d7c;
void FUN_8005ffac(VerticalMenu *);
int FUN_8005fd50(int);
int FUN_800601fc(int);
/* PC: GRAPHICS (the display mode and resolution, PC_GraphicsOptions in
   front/controlleroptions.cpp) between SCORE DISPLAY and LOAD DATA. The trick tips and score
   display toggles keep their rows (FUN_8005ffac edits rows 3 and 4); ids are 81 + row. */
extern "C" void PC_GraphicsOptions(int);
extern "C" {
	static char s_graphics[] = "GRAPHICS";
}
extern SMenuText7 DAT_800a1df8;
extern SMenuText5 DAT_800a1e4c;
int FUN_800609c8(int a);
int FUN_800605c0(int a);
extern SMenuText7 DAT_800a1eb4;
int FUN_80060738(int, int *);
extern SMenuText7 DAT_800a1f34;
extern int DAT_800d1d7c;
extern C30 DAT_800a1fd0;
extern C28 DAT_800a1ff0;
/* the title screen's sprite and its rectangle */
extern Sprite *DAT_800d1d88;
extern short DAT_800d1d8c;
extern short DAT_800d1d8e;
extern short DAT_800d1d90;
extern short DAT_800d1d92;
extern int DAT_800d1d84;

/* Front_InitBoardPictures2P__Fv (0x80050c5c) */
/* C++ (front end, 2-player board select). The 2-player twin of Front_InitBoardPictures1P (boardload1p.cpp),
   called once by Front_BoardSelect2P (boardselect2p.cpp) before its loop: fills in each player's picture
   file names for the three columns (the small "2" pictures: a deck file's second letter becomes
   '2'; locked decks show "b2lock01.bmp"), resets the streaming state that Front_StreamBoardPictures2P
   (boardstream2p.cpp) works through, and loads the "locked" picture. */
void Front_InitBoardPictures2P(void) {
	DAT_800d2c1c[0] = 8;
	DAT_800d2c1c[1] = 3;
	DAT_800d2c1c[2] = 15;
	DAT_800d333c[0][0] = -1;
	DAT_800d333c[1][0] = 0;

	for (int p = 0; p < 2; p++) {
		DAT_800d337c[p][0] = 0;
		DAT_800d335c[p][0] = GCareer.LastDeck[0];
		DAT_800d333c[p][1] = 0;
		DAT_800d337c[p][1] = 0;
		DAT_800d335c[p][1] = GCareer.LastTrucks[0];
		DAT_800d333c[p][2] = 0;
		DAT_800d337c[p][2] = 0;
		DAT_800d335c[p][2] = GCareer.LastWheel[0];
		DAT_800d22c8 = 1;

		for (int i = 0; i < DAT_800d2c1c[0]; i++) {
			if (Career_Progress(DAT_800d22d0[p])->NumTapes >=
			    Stats[DAT_800d22d0[p]].Boards[i].Req) {
				strcpy(DAT_800d2c2c[p][0][i], Stats[DAT_800d22d0[p]].Boards[i].File);

				DAT_800d2c2c[p][0][i][1] = '2';
			} else {
				strcpy(DAT_800d2c2c[p][0][i], "b2lock01.bmp");
			}

			DAT_800d31cc[p][0][i] = 0;
		}

		char *trucks[3] = { "T2_loose.bmp", "T2_mdium.bmp", "T2_tight.bmp" };

		for (int j = 0; j < DAT_800d2c1c[1]; j++) {
			strcpy(DAT_800d2c2c[p][1][j], trucks[j]);

			DAT_800d31cc[p][1][j] = 0;
		}

		char *wheels[15] = {
			"W2blue01.bmp", "W2blue02.bmp", "W2blue03.bmp", "W2blue04.bmp", "W2brow01.bmp",
			"W2dark01.bmp", "W2gree01.bmp", "W2gree02.bmp", "W2gree03.bmp", "W2oran01.bmp",
			"W2purp01.bmp", "W2red_01.bmp", "W2red_02.bmp", "W2whit01.bmp", "W2yell01.bmp",
		};

		for (int j = 0; j < DAT_800d2c1c[2]; j++) {
			strcpy(DAT_800d2c2c[p][2][j], wheels[j]);

			DAT_800d31cc[p][2][j] = 0;
		}

		DAT_800d22c0[p] = 0;
		DAT_800d22b8[p] = 0;
	}

	DAT_800d22cc = 0;
	DAT_800d22ac = 0;
	DAT_800d22c0[0] = (char *)Mem_NewX(FileIO_Open("b2lock01.bmp"), 1, 1);

	FileIO_Load(DAT_800d22c0[0]);
	FileIO_Sync();

	ushort clut;
	char *data;
	int w, h;

	Load8BitBMP2(DAT_800d22c0[0], &data, &w, &h, &clut);

	DAT_800d22c0[0] = 0;
	DAT_800d22b0 = new SlicedImage(data, w, h, 128, 128, 8, clut);

	DAT_800d22b0->pack();
}

/* Front_StreamBoardPictures2P__Fv (0x8005115c) */
/* C++ (front end, 2-player board select). Called once a frame by Front_BoardSelect2P (boardselect2p.cpp)
   until every picture is in: streams the deck/trucks/wheels pictures from CD one at a time,
   alternating players and cycling each player's column. The finished load is turned into a
   SlicedImage; the next file is picked outward from the column's current entry (offset 0, n-1, 1,
   n-2, ...) until the middle is reached. Sets DAT_800d22cc when both players' columns are done. */
void Front_StreamBoardPictures2P(void) {
	PC_FramePoll(); /* a frame due while the pictures load runs here */

	int col = DAT_800d22b8[DAT_800d22ac];

	FileIO_Continue();

	if (DAT_800d22cc)
		DAT_800d22c8 = 1;

	if (DAT_800d22c8 == 1 && !Loading && !DAT_800d22cc) {
		if (DAT_800d333c[DAT_800d22ac][col] >= 0) {
			ushort clut;
			char *data;
			int w, h;

			Load8BitBMP2(DAT_800d22c0[DAT_800d22ac], &data, &w, &h, &clut);

			DAT_800d22c0[DAT_800d22ac] = 0;
			DAT_800d31cc[DAT_800d22ac][col][DAT_800d22a8] =
			    new SlicedImage(data, w, h, 128, 128, 8, clut);
		}

		if (!DAT_800d337c[DAT_800d22ac][col]) {
			int off = DAT_800d333c[DAT_800d22ac][col];

			if (off == -1) {
				DAT_800d333c[DAT_800d22ac][col] = 0;
			} else {
				int n = DAT_800d2c1c[col];

				if (off < n / 2)
					DAT_800d333c[DAT_800d22ac][col] = n - off - 1;
				else if (off > n / 2)
					DAT_800d333c[DAT_800d22ac][col] = n - off;
				else
					DAT_800d337c[DAT_800d22ac][col] = 1;
			}
		}

		DAT_800d22ac = 1 - DAT_800d22ac;
		int k;

		for (k = 0; k < 3; k++) {
			if (++DAT_800d22b8[DAT_800d22ac] >= 3)
				DAT_800d22b8[DAT_800d22ac] = 0;

			if (!DAT_800d337c[DAT_800d22ac][DAT_800d22b8[DAT_800d22ac]])
				break;
		}

		if (k == 3) {
			if (DAT_800d22ac == 1)
				DAT_800d22cc = 1;
		} else {
			int c = DAT_800d22b8[DAT_800d22ac];

			DAT_800d22a8 = DAT_800d335c[DAT_800d22ac][c] + DAT_800d333c[DAT_800d22ac][c];

			if (DAT_800d22a8 >= DAT_800d2c1c[c])
				DAT_800d22a8 -= DAT_800d2c1c[c];

			int size = FileIO_Open(DAT_800d2c2c[DAT_800d22ac][c][DAT_800d22a8]);

			if (size > 0)
				FileIO_Load(DAT_800d22c0[DAT_800d22ac] = (char *)Mem_NewX(size, 1, 1));
		}
	}
}

/* Front_FreeBoardPictures2P__Fv (0x800514f0) */
/* a CClass object: its vptr at +0 (deleted through its vtable, pc_vdelete) */
void Front_FreeBoardPictures2P(void) {
	int k, i, j;
	SPsxVtObject *o;

	if (Loading)
		FileIO_Sync();

	for (k = 0; k < 2; k++) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < DAT_800d2c1c[i]; j++) {
				o = (SPsxVtObject *)DAT_800d31cc[k][i][j];

				if (o) {
					pc_vdelete(o->vt, 0, o);

					DAT_800d31cc[k][i][j] = 0;
				}
			}
		}

		if (DAT_800d22c0[k])
			Mem_Delete(DAT_800d22c0[k]);
	}

	o = (SPsxVtObject *)DAT_800d22b0;

	if (o)
		pc_vdelete(o->vt, 0, o);
}

/* Front_BoardSelect2P__Fi (0x80051660) */
/* C++ (front end, 2-player board select). The 2-player twin of Front_BoardSelect1P (boardselect1p.cpp):
   every piece of per-screen state is an array indexed by player. Each player has three columns
   (deck, trucks, wheels) scrolling pairs of SlicedImage pictures through a clipped box, a sliding
   text bar with the deck's name (or "Get %d tapes"), and four stat rows of bar sprites. X/Start
   locks in an unlocked deck ("board chosen" pops up over a saved screen rectangle), triangle
   backs out. Returns 1 once both players have chosen, -1 if both back out. No demo twin. */
int Front_BoardSelect2P(int player) {
	int x[2][3][2];
	int y[2][3][2];
	int idx[2][3][2];
	SlicedImage *img[2][3][2];
	int xoff[2][3];
	int yoff[2][3];
	int boxX[2][3];
	int boxY[2][3];
	int boxW[2][3];
	int boxH[2][3];
	int deck[2];
	int trucks[2];
	Sprite *box[3];
	Sprite *bar[5];
	Sprite *row[5];
	int base[2][5];
	int statsDone[2];
	int statsTimer[2];
	int i;

	FrontLoader_LoadSet(10);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("select2b.bmp"));
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	Pack_DefragmentFully();

	for (int p = 0; p < 2; p++) {
		x[p][0][0] = DAT_800d1bcc[p];
		x[p][0][1] = DAT_800d1bcc[p];
		y[p][0][0] = 119;
		y[p][0][1] = 265;
		idx[p][0][0] = GCareer.LastDeck[p];
		idx[p][0][1] = GCareer.LastDeck[p] + 1;

		if (idx[p][0][1] == DAT_800d2c1c[0])
			idx[p][0][1] = 0;

		img[p][0][0] = 0;
		img[p][0][1] = 0;
		xoff[p][0] = 21;
		yoff[p][0] = 9;
		boxX[p][0] = DAT_800d1bcc[p];
		boxY[p][0] = 119;
		boxW[p][0] = 74;
		boxH[p][0] = 146;

		x[p][1][0] = DAT_800d1bdc[p];
		x[p][1][1] = DAT_800d1bdc[p];
		y[p][1][0] = 190;
		y[p][1][1] = 254;
		idx[p][1][0] = GCareer.LastTrucks[p];
		idx[p][1][1] = GCareer.LastTrucks[p] + 1;

		if (idx[p][1][1] == DAT_800d2c1c[1])
			idx[p][1][1] = 0;

		img[p][1][0] = 0;
		img[p][1][1] = 0;
		xoff[p][1] = 8;
		yoff[p][1] = 8;
		boxX[p][1] = DAT_800d1bdc[p];
		boxY[p][1] = 190;
		boxW[p][1] = 76;
		boxH[p][1] = 64;

		x[p][2][0] = DAT_800d1bec[p];
		x[p][2][1] = DAT_800d1bec[p];
		y[p][2][0] = 209;
		y[p][2][1] = 265;
		idx[p][2][0] = GCareer.LastWheel[p];
		idx[p][2][1] = GCareer.LastWheel[p] + 1;

		if (idx[p][2][1] == DAT_800d2c1c[2])
			idx[p][2][1] = 0;

		img[p][2][0] = 0;
		img[p][2][1] = 0;
		xoff[p][2] = 16;
		yoff[p][2] = 12;
		boxX[p][2] = DAT_800d1bec[p];
		boxY[p][2] = 209;
		boxW[p][2] = 64;
		boxH[p][2] = 56;
	}

	DAT_800d22d0[0] = player;
	DAT_800d22d0[1] = SelectedSkater2;
	deck[0] = idx[0][0][0];
	deck[1] = idx[1][0][0];
	trucks[0] = idx[0][1][0];
	trucks[1] = idx[1][1][0];

	box[0] = new Sprite(DAT_800d1bcc[0], 119, 74, 146, 0, 64, 96);
	box[1] = new Sprite(DAT_800d1bdc[0], 190, 76, 64, 0, 64, 62);
	box[2] = new Sprite(DAT_800d1bec[0], 209, 64, 56, 0, 64, 62);

	for (i = 0; i < 5; i++) {
		char name[16];

		if (i < 4)
			sprintf(name, "bar%d.bmp", i + 1);
		else
			sprintf(name, "bar9.bmp");

		bar[i] = new Sprite(name, 1, 32, 128);
		row[i] = new Sprite(DAT_800d1bc4[0], i * 22 + 294, 200, 20, 1, 32, 128);
	}

	bar[4]->prepare();

	/* best unlocked board per player (computed but unused in 2P). The loop bound in a variable
	   set outside the player loop keeps cse1 from folding the entry test; retail's
	   `li t9,1; beq t9,zero` guard is the PRE copy of it, rematerialized after spilling. */
	int n[2];
	int lastBoard = 7;

	for (int p = 0; p < 2; p++) {
		base[p][0] = Stats[DAT_800d22d0[p]].Stats[0];
		base[p][1] = Stats[DAT_800d22d0[p]].Stats[1];
		base[p][2] = Stats[DAT_800d22d0[p]].Stats[2];
		base[p][3] = Stats[DAT_800d22d0[p]].Stats[3];
		base[p][4] = 0;
		n[p] = 0;

		while (n[p] < lastBoard) {
			if (Career_Progress(DAT_800d22d0[p])->NumTapes <
			    Stats[DAT_800d22d0[p]].Boards[n[p] + 1].Req)
				break;

			n[p]++;

			if (n[p] >= 7)
				break;
		}

		base[p][4] = DAT_800be42c[trucks[p]];
		statsDone[p] = 0;
		statsTimer[p] = 2;
	}

	int state[2];
	int timer[2];
	int col[2];
	int dir[2];
	int speed[2];
	int chosen[2];

	for (int p = 0; p < 2; p++) {
		state[p] = 1;
		timer[p] = 3;
		col[p] = 0;
		dir[p] = 0;
		speed[p] = 0;
		chosen[p] = 0;
	}

	SLabel6 sBoard = "board";
	SLabel7 sChosen = "chosen";
	int lx[2];
	int ly[2];
	EraseSprite *popup[2];

	lx[0] = DAT_800d1be4[0];
	lx[1] = DAT_800d1be4[1];
	ly[0] = 180;
	ly[1] = 200;
	popup[0] = 0;
	popup[1] = 0;

	Mess_SetCurrentFont("mainf.fnt");
	Mess_SetFontScale(200);
	Mess_SetTextJustify(1);

	int textState[2];
	int textTimer[2];
	int savedTextState[2];
	int savedTextTimer[2];
	char text[2][48];
	int textX[2];
	int textY[2];
	int textSpeed[2];
	Sprite *textBar[2];

	for (int p = 0; p < 2; p++) {
		textState[p] = 3;
		textTimer[p] = 2;
		savedTextState[p] = 2;
		savedTextTimer[p] = 10;
		strcpy(text[p], Stats[DAT_800d22d0[p]].Boards[deck[p]].Name);

		if (p == 0)
			textX[p] = -Mess_TextWidth(text[0]);
		else
			textX[p] = 640;

		{
			int h = Mess_TextHeight(text[p]);

			textY[p] = (h >> 1) + 75 + Mess_TextHeightBelowBaseline(text[p]);
		}

		textSpeed[p] = (DAT_800d1bbc[p] - textX[p]) / 10;
		textX[p] = DAT_800d1bbc[p] - textSpeed[p] * 10;
		textBar[p] = new Sprite(DAT_800d1bac[p], 50, DAT_800d1bb4[p], 41, 1, 32, 128);
	}

	Sprite *title[3];

	title[0] = new Sprite("Box2_01.bmp", 1, 32, 128);
	title[1] = new Sprite("Box2_02.bmp", 1, 32, 128);
	title[2] = new Sprite("Box2_03.bmp", 1, 32, 128);
	PadBuffer *input[2];

	input[0] = new PadBuffer(0);
	input[1] = new PadBuffer(1);

	FrontLoader_SetFiles(3);
	Front_InitBoardPictures2P();

	int result = 0;

	while (!result) {
		Front_UpdateMusicLoop();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		PollHost();

		if (DAT_800d22cc) {
			DAT_800d22b0->unpack();
			FrontLoader_Continue();
		} else {
			Front_StreamBoardPictures2P();
		}

		HiRes_ClearOTag();
		ResetOT();

		for (int p = 0; p < 2; p++) {
			int c = col[p];
			int cnt = 0;
			int k;

			if (chosen[p] < 4)
				cnt = 3;

			for (k = 0; k < cnt; k++) {
				c++;

				if (c >= 3)
					c = 0;

				if (c != col[p]) {
					int j;

					if (state[p] == 1)
						box[c]->draw(boxX[p][c], boxY[p][c], 0, -1, -1, -1);

					j = y[p][c][0] >= y[p][c][1];

					if (img[p][c][j] == 0) {
						SlicedImage *im = DAT_800d31cc[p][c][idx[p][c][j]];

						if (im) {
							img[p][c][j] = im;
							im->pack();
						}
					}

					if (img[p][c][j]) {
						if (state[p] == 1)
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 1);
						else
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
					} else if (c == 0) {
						DAT_800d22b0->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0, -1,
						                   -1, -1, 0, 0);
					}
				} else {
					int j;

					title[c]->draw(boxX[p][c], boxY[p][c], 0, -1, -1, -1);
					SetClippingBox(boxX[p][c] + 3, boxY[p][c] + 5, boxW[p][c] - 6, boxH[p][c] - 10);

					for (j = 0; j < 2; j++) {
						if (img[p][c][j] == 0) {
							SlicedImage *im = DAT_800d31cc[p][c][idx[p][c][j]];

							if (im) {
								img[p][c][j] = im;
								im->pack();
							}
						}

						if (img[p][c][j])
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
						else if (c == 0)
							DAT_800d22b0->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
					}

					SetClippingBox(0, 0, 640, 480);
				}
			}

			if (textState[p] != 3 && textState[p] != 1) {
				textBar[p]->draw(DAT_800d1bac[p], 50, 0, -1, -1, -1);
				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(200);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(1);
				Mess_DrawText(textX[p], textY[p], text[p], 0, 0, 256, 0);
			}

			int ry = 294;

			if (!statsDone[p]) {
				int r;

				for (r = 0; r < 5; r++) {
					int bx;

					row[r]->draw(DAT_800d1bc4[p], ry, 0, -1, -1, -1);

					bx = DAT_800d1bc4[p];

					for (int b = 0; b < base[p][r]; b++) {
						bar[r]->draw(bx, ry, 0, -1, -1, -1);

						bx += 15;
					}

					ry += 22;
				}
			}

			if (chosen[p] == 4) {
				if (popup[p] == 0)
					popup[p] = new EraseSprite(DAT_800d1be4[p] - 40, 150, 80, 80);

				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(150);
				Mess_SetRGB(250, 190, 60, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(lx[p], ly[0], sBoard, 1, 0, 256, 0);
				Mess_DrawText(lx[p], ly[1], sChosen, 1, 0, 256, 0);
			}

			if (chosen[p] == 5) {
				popup[p]->restore(DAT_800d1be4[p] - 40, 150);
				delete popup[p];
				popup[p] = 0;
				chosen[p] = 0;
			}
		}

		HiRes_DrawOTag();

		for (int p = 0; p < 2; p++) {
			int again;

			input[p]->update();

			do {
				again = 0;

				switch (state[p]) {
					case 0:
						if (DAT_800d22c8 > 1) {
							DAT_800d22c8--;
						} else if (chosen[p] >= 4) {
							input[p]->reset();
						} else {
							if (chosen[p] > 0)
								chosen[p]++;

							int press = input[p]->getPress();

							if (press)
								DAT_800d22c8 = 3;

							if (press == 1) {
								SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

								timer[p] = 4;
								state[p] = 1;
								dir[p] = -1;
								again = 1;
							} else if (press == 2) {
								SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

								timer[p] = 4;
								state[p] = 1;
								dir[p] = 1;
								again = 1;
							} else if (press == 3) {
								int k;

								if (col[p] == 0)
									SFX_PlayX(SND_SELECTBOARD3, 0x2000, 0);
								else if (col[p] == 1)
									SFX_PlayX(SND_SELECTTRUCK2, 0x2000, 0);
								else if (col[p] == 2)
									SFX_PlayX(SND_SELECTWHEEL, 0x2000, 0);

								timer[p] = 10;
								state[p] = 2;
								speed[p] = boxH[p][col[p]] / 10;
								k = y[p][col[p]][0] <= y[p][col[p]][1];
								y[p][col[p]][k] = boxY[p][col[p]] - boxH[p][col[p]];
								idx[p][col[p]][k] -= 2;

								if (idx[p][col[p]][k] < 0)
									idx[p][col[p]][k] += DAT_800d2c1c[col[p]];

								if (img[p][col[p]][k])
									img[p][col[p]][k]->unpack();

								img[p][col[p]][k] = 0;
							} else if (press == 4) {
								if (col[p] == 0)
									SFX_PlayX(SND_SELECTBOARD3, 0x2000, 0);
								else if (col[p] == 1)
									SFX_PlayX(SND_SELECTTRUCK2, 0x2000, 0);
								else if (col[p] == 2)
									SFX_PlayX(SND_SELECTWHEEL, 0x2000, 0);

								timer[p] = 10;
								state[p] = 2;
								speed[p] = -boxH[p][col[p]] / 10;
							}
						}

						break;
					case 1:
						if (--timer[p] == 3) {
							col[p] += dir[p];

							if (col[p] < 0)
								col[p] = 2;

							if (col[p] >= 3)
								col[p] = 0;

							savedTextState[p] = textState[p];
							savedTextTimer[p] = textTimer[p];
							textState[p] = 3;
							textTimer[p] = 2;
						}

						if (timer[p] == 0) {
							again = 1;
							state[p] = 0;
						}

						break;
					case 2:
						if (timer[p] == 10 && col[p] == 0) {
							textState[p] = 4;
							textTimer[p] = 1;
						}

						{
							int j;

							for (j = 0; j < 2; j++)
								y[p][col[p]][j] += speed[p];
						}

						if (--timer[p] == 0) {
							int k = y[p][col[p]][0] >= y[p][col[p]][1];

							if (speed[p] < 0) {
								y[p][col[p]][k] = boxY[p][col[p]] + boxH[p][col[p]];
								idx[p][col[p]][k] += 2;

								if (idx[p][col[p]][k] >= DAT_800d2c1c[col[p]])
									idx[p][col[p]][k] -= DAT_800d2c1c[col[p]];

								if (img[p][col[p]][k])
									img[p][col[p]][k]->unpack();

								img[p][col[p]][k] = 0;
								k = 1 - k;
								y[p][col[p]][k] = boxY[p][col[p]];

								if (col[p] == 0)
									deck[p]++;

								if (deck[p] >= 8)
									deck[p] = 0;

								if (col[p] == 1)
									trucks[p]++;

								if (trucks[p] >= 3)
									trucks[p] = 0;
							} else {
								y[p][col[p]][k] = boxY[p][col[p]];
								y[p][col[p]][1 - k] = boxY[p][col[p]] + boxH[p][col[p]];

								if (col[p] == 0)
									deck[p]--;

								if (deck[p] < 0)
									deck[p] = 7;

								if (col[p] == 1)
									trucks[p]--;

								if (trucks[p] < 0)
									trucks[p] = 2;
							}

							state[p] = 0;
							again = 1;

							if (col[p] == 1)
								statsDone[p] = 0;
						}

						break;
				}
			} while (again);

			switch (textState[p]) {
				case 2:
					textTimer[p]--;
					textX[p] += textSpeed[p];

					if (textTimer[p] == 0) {
						textState[p] = 0;
						textTimer[p] = 2;
					}

					break;
				case 3:
					if (--textTimer[p] == 0) {
						textState[p] = savedTextState[p];
						textTimer[p] = savedTextTimer[p];
					}

					break;
				case 4:
					if (--textTimer[p] == 0) {
						textSpeed[p] = -textSpeed[p];
						textState[p] = 5;
						textTimer[p] = 10;
					}

					break;
				case 5:
					textTimer[p]--;
					textX[p] += textSpeed[p];

					if (textTimer[p] == 0) {
						if (Career_Progress(DAT_800d22d0[p])->NumTapes >=
						    Stats[DAT_800d22d0[p]].Boards[deck[p]].Req)
							strcpy(text[p], Stats[DAT_800d22d0[p]].Boards[deck[p]].Name);
						else
							sprintf(text[p], "Get %d tapes",
							        Stats[DAT_800d22d0[p]].Boards[deck[p]].Req);

						Mess_SetCurrentFont("mainf.fnt");

						if (p == 0)
							textX[p] = -Mess_TextWidth(text[0]);
						else
							textX[p] = 640;

						{
							int h = Mess_TextHeight(text[p]);

							textY[p] = (h >> 1) + 75 + Mess_TextHeightBelowBaseline(text[p]);
						}

						textSpeed[p] = (DAT_800d1bbc[p] - textX[p]) / 10;
						textX[p] = DAT_800d1bbc[p] - textSpeed[p] * 10;
						textState[p] = 2;
						textTimer[p] = 10;
					}

					break;
				case 0:
					if (--textTimer[p] == 0) {
						textState[p] = 1;
						statsDone[p] = 0;
						statsTimer[p] = 3;
					}

					break;
			}

			if (!statsDone[p]) {
				base[p][4] = DAT_800be42c[trucks[p]];

				if (--statsTimer[p] == 0)
					statsDone[p] = 1;
			}

			if (Pad[p].MenuStart.Triggered || Pad[p].MenuAccept.Triggered) {
				Pad_ClearTriggers(&Pad[p]);

				GBoardStats[p].s[0] = 3;
				GBoardStats[p].s[1] = 3;
				GBoardStats[p].s[2] = 3;
				GBoardStats[p].s[3] = 3;
				GBoardStats[p].s[4] = DAT_800be42c[trucks[p]];

				if (Career_Progress(DAT_800d22d0[p])->NumTapes >=
				    Stats[DAT_800d22d0[p]].Boards[deck[p]].Req) {
					int j;

					GCareer.LastDeck[p] = deck[p];
					j = y[p][1][0] >= y[p][1][1];
					GCareer.LastTrucks[p] = idx[p][1][j];
					j = y[p][2][0] >= y[p][2][1];
					GCareer.LastWheel[p] = idx[p][2][j];

					if (chosen[p] == 0) {
						if (speed[p] > 0 && col[p] == 0 && timer[p] != 0) {
							timer[p] = 10 - timer[p];
							speed[p] = -speed[p];

							if (textState[p] == 5)
								textTimer[p] += 8;

							if (--deck[p] < 0)
								deck[p] = 7;
						} else if (speed[p] < 0 && col[p] == 0 && timer[p] != 0) {
							timer[p] = 10 - timer[p];
							speed[p] = -speed[p];

							if (textState[p] == 5)
								textTimer[p] += 8;

							if (++deck[p] >= 8)
								deck[p] = 0;
						}

						SFX_PlayX(SND_SELECT1, 0x2000, 0);

						chosen[p] = 1;
					}
				} else {
					SFX_PlayX(SND_NOSELECT1, 0x2000, 0);
				}

				if (chosen[0] && chosen[1])
					result = 1;
			}

			if (Pad[p].MenuBack.Triggered) {
				if (chosen[p] || !chosen[1 - p])
					SFX_PlayX(SND_GOBACK2, 0x2000, 0);

				Pad_ClearTriggers(&Pad[p]);

				if (!chosen[0] && !chosen[1])
					result = -1;

				if (chosen[p] == 4)
					chosen[p] = 5;
			}
		}
	}

	HiRes_BlankScreen();
	SpoolerBlocksFileIO->unloadPRE("Equip2.pre");

	delete popup[0];
	delete popup[1];
	delete input[0];
	delete input[1];
	delete textBar[0];
	delete textBar[1];

	for (i = 0; i < 3; i++) {
		delete box[i];
		delete title[i];
	}

	for (i = 0; i < 5; i++) {
		delete bar[i];
		delete row[i];
	}

	Front_FreeBoardPictures2P();

	return result;
}

/* Front_InitBoardPictures1P__Fv (0x80053dbc) */
/* C++ (front end, 1-player board select). Called once by Front_BoardSelect1P (boardselect1p.cpp) before
   its loop: fills in the picture file names of the three columns (decks the skater hasn't got the
   tapes for show "b1lock01.bmp"; trucks and wheels come from fixed tables), resets the streaming
   state that Front_StreamBoardPictures1P (boardstream1p.cpp) works through, and loads the "locked" picture. */
void Front_InitBoardPictures1P(void) {
	DAT_800d3394[0] = -1;
	DAT_800d33c4[0] = 8;
	DAT_800d33b4[0] = 0;
	DAT_800d3394[1] = 0;
	DAT_800d33c4[1] = 3;
	DAT_800d33b4[1] = 0;
	DAT_800d3394[2] = 0;
	DAT_800d33c4[2] = 15;
	DAT_800d33b4[2] = 0;
	DAT_800d22e8 = 1;
	DAT_800d33a4[0] = GCareer.LastDeck[0];
	DAT_800d33a4[1] = GCareer.LastTrucks[0];
	DAT_800d33a4[2] = GCareer.LastWheel[0];

	for (int i = 0; i < DAT_800d33c4[0]; i++) {
		if (Career_Progress(DAT_800d22f0)->NumTapes >= Stats[DAT_800d22f0].Boards[i].Req)
			strcpy(DAT_800d33d4[0][i], Stats[DAT_800d22f0].Boards[i].File);
		else
			strcpy(DAT_800d33d4[0][i], "b1lock01.bmp");

		DAT_800d36a4[0][i] = 0;
	}

	char *trucks[3] = { "T1_loose.bmp", "T1_mdium.bmp", "T1_tight.bmp" };

	for (int j = 0; j < DAT_800d33c4[1]; j++) {
		strcpy(DAT_800d33d4[1][j], trucks[j]);

		DAT_800d36a4[1][j] = 0;
	}

	char *wheels[15] = {
		"W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp", "W1brow01.bmp",
		"W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp", "W1gree03.bmp", "W1oran01.bmp",
		"W1purp01.bmp", "W1red_01.bmp", "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp",
	};

	for (int j = 0; j < DAT_800d33c4[2]; j++) {
		strcpy(DAT_800d33d4[2][j], wheels[j]);

		DAT_800d36a4[2][j] = 0;
	}

	DAT_800d22ec = 0;
	DAT_800d22e4 = 0;
	DAT_800d22e0 = 0;
	DAT_800d22e4 = (char *)Mem_NewX(FileIO_Open("b1lock01.bmp"), 1, 1);

	FileIO_Load(DAT_800d22e4);
	FileIO_Sync();

	ushort clut;
	char *data;
	int w, h;

	Load8BitBMP2(DAT_800d22e4, &data, &w, &h, &clut);

	DAT_800d22e4 = 0;
	DAT_800d22dc = new SlicedImage(data, w, h, 128, 128, 8, clut);

	DAT_800d22dc->pack();
}

/* Front_StreamBoardPictures1P__Fv (0x80054134) */
/* C++ (front end, 1-player board select). The 1-player twin of Front_StreamBoardPictures2P (boardstream2p.cpp),
   called once a frame by Front_BoardSelect1P (boardselect1p.cpp) until every picture is in: streams the
   deck/trucks/wheels pictures from CD one at a time, cycling the columns. The finished load is
   turned into a SlicedImage; the next file is picked outward from the column's current entry
   (offset 0, n-1, 1, n-2, ...) until the middle is reached. Sets DAT_800d22ec when all are done. */
void Front_StreamBoardPictures1P(void) {
	int col = DAT_800d22e0;

	FileIO_Continue();

	if (DAT_800d22ec)
		DAT_800d22e8 = 1;

	if (DAT_800d22e8 == 1 && !Loading && !DAT_800d22ec) {
		if (DAT_800d3394[col] >= 0) {
			ushort clut;
			char *data;
			int w, h;

			Load8BitBMP2(DAT_800d22e4, &data, &w, &h, &clut);

			DAT_800d22e4 = 0;
			DAT_800d36a4[col][DAT_800d22d8] = new SlicedImage(data, w, h, 128, 128, 8, clut);
		}

		if (!DAT_800d33b4[col]) {
			int off = DAT_800d3394[col];

			if (off == -1) {
				DAT_800d3394[col] = 0;
			} else {
				int n = DAT_800d33c4[col];

				if (off < n / 2)
					DAT_800d3394[col] = n - off - 1;
				else if (off > n / 2)
					DAT_800d3394[col] = n - off;
				else
					DAT_800d33b4[col] = 1;
			}
		}

		int k;

		for (k = 0; k < 3; k++) {
			if (++DAT_800d22e0 >= 3)
				DAT_800d22e0 = 0;

			if (!DAT_800d33b4[DAT_800d22e0])
				break;
		}

		if (k == 3) {
			DAT_800d22ec = 1;
		} else {
			int c = DAT_800d22e0;

			DAT_800d22d8 = DAT_800d33a4[c] + DAT_800d3394[c];

			if (DAT_800d22d8 >= DAT_800d33c4[c])
				DAT_800d22d8 -= DAT_800d33c4[c];

			int size = FileIO_Open(DAT_800d33d4[c][DAT_800d22d8]);

			if (size > 0)
				FileIO_Load(DAT_800d22e4 = (char *)Mem_NewX(size, 1, 1));
		}
	}
}

/* Front_FreeBoardPictures1P__Fv (0x800543d4) */
/* a CClass object: its vptr at +0 (deleted through its vtable, pc_vdelete) */
void Front_FreeBoardPictures1P(void) {
	int i, j;
	SPsxVtObject *o;

	if (Loading)
		FileIO_Sync();

	for (i = 0; i < 3; i++) {
		for (j = 0; j < DAT_800d33c4[i]; j++) {
			o = (SPsxVtObject *)DAT_800d36a4[i][j];

			if (o) {
				pc_vdelete(o->vt, 0, o);

				DAT_800d36a4[i][j] = 0;
			}
		}
	}

	if (DAT_800d22e4)
		Mem_Delete(DAT_800d22e4);

	o = (SPsxVtObject *)DAT_800d22dc;

	if (o)
		pc_vdelete(o->vt, 0, o);
}

/* Front_BoardSelect1P__Fi (0x80054510) */
/* C++ (front end, 1-player board select; called from the screen dispatcher Front_Main). Three
   columns (deck, trucks, wheels) each scroll a pair of SlicedImage pictures vertically through a
   clipped box; left/right moves between columns, up/down scrolls the selected column. A text bar
   shows the deck's name (or "Get %d tapes to unlock" / "Unlock in career mode"), and five stat
   rows draw bar sprites: the skater's base stats, the best unlocked board's, and the difference
   to the chosen deck. X/Start accepts an unlocked deck and saves the choice into the career
   record; triangle backs out. Returns 1 (accept) or -1 (back). No demo twin. */
int Front_BoardSelect1P(int player) {
	int x[3][2];
	int y[3][2];
	int idx[3][2];
	SlicedImage *img[3][2];
	int xoff[3];
	int yoff[3];
	int boxX[3];
	int boxY[3];
	int boxW[3];
	int boxH[3];
	Sprite *box[3];
	Sprite *bar[5];
	Sprite *row[5];
	int base[5];
	int board[5];
	int delta[5];
	int deck;
	int trucks;
	int n;
	int col;
	int i;

	FrontLoader_LoadSet(9);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("select_b.bmp"));
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	Pack_DefragmentFully();
	Mess_SetRGB(128, 128, 128, 0);
	Mess_SetFontScale(256);
	Mess_SetTextJustify(1);

	x[0][1] = x[0][0] = 144;
	y[0][0] = 117;
	y[0][1] = 397;
	idx[0][0] = GCareer.LastDeck[0];
	idx[0][1] = GCareer.LastDeck[0] + 1;

	if (idx[0][1] == DAT_800d33c4[0])
		idx[0][1] = 0;

	img[0][1] = img[0][0] = 0;
	xoff[0] = 31;
	yoff[0] = 12;
	boxX[0] = 144;
	boxY[0] = 117;
	boxW[0] = 127;
	boxH[0] = 280;

	x[1][1] = x[1][0] = 289;
	y[1][0] = 261;
	y[1][1] = 395;
	idx[1][0] = GCareer.LastTrucks[0];
	idx[1][1] = GCareer.LastTrucks[0] + 1;

	if (idx[1][1] == DAT_800d33c4[1])
		idx[1][1] = 0;

	img[1][1] = img[1][0] = 0;
	xoff[1] = 13;
	yoff[1] = 17;
	boxX[1] = 289;
	boxY[1] = 261;
	boxW[1] = 155;
	boxH[1] = 134;

	x[2][1] = x[2][0] = 429;
	y[2][0] = 273;
	y[2][1] = 383;
	idx[2][0] = GCareer.LastWheel[0];
	idx[2][1] = GCareer.LastWheel[0] + 1;

	if (idx[2][1] == DAT_800d33c4[2])
		idx[2][1] = 0;

	img[2][1] = img[2][0] = 0;
	xoff[2] = 27;
	yoff[2] = 23;
	boxX[2] = 429;
	boxY[2] = 273;
	boxW[2] = 118;
	boxH[2] = 110;

	DAT_800d22f0 = player;
	deck = idx[0][0];
	trucks = idx[1][0];

	box[0] = new Sprite(144, 117, 127, 280, 0, 64, 96);
	box[1] = new Sprite(289, 261, 155, 134, 0, 64, 62);
	box[2] = new Sprite(429, 273, 118, 110, 0, 64, 62);

	for (i = 0; i < 5; i++) {
		char name[16];

		if (i < 4)
			sprintf(name, "bar%d.bmp", i + 1);
		else
			sprintf(name, "bar9.bmp");

		bar[i] = new Sprite(name, 1, 32, 128);
		row[i] = new Sprite(398, i * 24 + 116, 200, 20, 1, 32, 128);
	}

	bar[4]->prepare();

	n = 0;
	base[4] = 0;
	base[0] = Stats[DAT_800d22f0].Stats[0];
	base[1] = Stats[DAT_800d22f0].Stats[1];
	base[2] = Stats[DAT_800d22f0].Stats[2];
	base[3] = Stats[DAT_800d22f0].Stats[3];

	while (n < 7) {
		if (Career_Progress(DAT_800d22f0)->NumTapes <
		    Stats[DAT_800d22f0].Boards[n + 1].Req)
			break;

		n++;

		if (n >= 7)
			break;
	}

	board[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
	board[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
	board[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
	board[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];
	board[4] = DAT_800be438[trucks];
	delta[0] = 0;
	delta[1] = 0;
	delta[2] = 0;
	delta[3] = 0;
	delta[4] = 0;
	int statsDone = 0;
	int statsTimer = 2;
	int state = 1;
	int timer = 3;

	col = 0;
	int dir = 0;
	int speed = 0;
	int textState = 3;
	int textTimer = 2;
	int savedTextState = 2;
	int savedTextTimer = 10;

	char text[48];

	strcpy(text, Stats[DAT_800d22f0].Boards[deck].Name);

	Sprite *textBar = new Sprite(0, 49, 585, 43, 1, 32, 128);

	Mess_SetCurrentFont("mainf.fnt");

	int textX = -Mess_TextWidth(text);
	int textY;

	{
		int h = Mess_TextHeight(text);

		textY = (h >> 1) + 71 + Mess_TextHeightBelowBaseline(text);
	}

	int textSpeed = (117 - textX) / 10;

	textX = 117 - textSpeed * 10;

	Sprite *title[3];

	title[0] = new Sprite("Box1_01.bmp", 1, 32, 128);
	title[1] = new Sprite("Box1_02.bmp", 1, 32, 128);
	title[2] = new Sprite("Box1_03.bmp", 1, 32, 128);
	PadBuffer *input = new PadBuffer(-1);
	int result;

	FrontLoader_SetFiles(3);
	Front_InitBoardPictures1P();

	while (1) {
		int again;

		Front_UpdateMusicLoop();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		PollHost();

		if (DAT_800d22ec) {
			DAT_800d22dc->unpack();
			FrontLoader_Continue();
		} else {
			Front_StreamBoardPictures1P();
		}

		HiRes_ClearOTag();
		ResetOT();

		{
			int c = col;
			int k;

			for (k = 0; k < 3; k++) {
				c++;

				if (c >= 3)
					c = 0;

				if (c != col) {
					int j;

					if (state == 1)
						box[c]->draw(boxX[c], boxY[c], 0, -1, -1, -1);

					j = y[c][0] >= y[c][1];

					if (img[c][j] == 0) {
						SlicedImage *im = DAT_800d36a4[c][idx[c][j]];

						if (im) {
							img[c][j] = im;
							im->pack();
						}
					}

					if (img[c][j]) {
						if (state == 1)
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                1);
						else
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                0);
					} else if (c == 0) {
						DAT_800d22dc->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
						                   0);
					}
				} else {
					int j;

					title[c]->draw(boxX[c], boxY[c], 0, -1, -1, -1);
					SetClippingBox(boxX[c] + 6, boxY[c] + 9, boxW[c] - 12, boxH[c] - 18);

					for (j = 0; j < 2; j++) {
						if (img[c][j] == 0) {
							SlicedImage *im = DAT_800d36a4[c][idx[c][j]];

							if (im) {
								img[c][j] = im;
								im->pack();
							}
						}

						if (img[c][j])
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                0);
						else if (c == 0)
							DAT_800d22dc->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1,
							                   0, 0);
					}

					SetClippingBox(0, 0, 640, 480);
				}
			}
		}

		if (textState != 3 && textState != 1) {
			textBar->draw(0, 49, 0, -1, -1, -1);
			Mess_SetCurrentFont("mainf.fnt");
			Mess_DrawText(textX, textY, text, 0, 0, 256, 0);
		}

		int ry = 116;

		if (!statsDone) {
			int r;

			for (r = 0; r < 5; r++) {
				int bx;

				row[r]->draw(398, ry, 0, -1, -1, -1);

				bx = 398;

				for (int b = 0; b < base[r]; b++) {
					bar[r & 3]->draw(bx, ry, 0, -1, -1, -1);
					bx += 15;
				}

				for (int b = 0; b < board[r]; b++) {
					bar[4]->draw(bx, ry, 0, -1, -1, -1);

					bx += 15;
				}

				for (int b = 0; b < delta[r]; b++) {
					bar[4]->drawFaded(bx, ry, 0, 64, 64, 64);

					bx += 15;
				}

				ry += 24;
			}
		}

		HiRes_DrawOTag();
		input->update();

		do {
			again = 0;

			switch (state) {
				case 0:
					if (DAT_800d22e8 > 1) {
						DAT_800d22e8--;
					} else {
						int press = input->getPress();

						if (press)
							DAT_800d22e8 = 3;

						if (press == 1) {
							SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

							timer = 4;
							state = 1;
							dir = -1;
							again = 1;
						} else if (press == 2) {
							SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

							timer = 4;
							state = 1;
							dir = 1;
							again = 1;
						} else if (press == 3) {
							int k;

							if (col == 0)
								SFX_PlayX(SND_SELECTBOARD3, 0x2000, 0);
							else if (col == 1)
								SFX_PlayX(SND_SELECTTRUCK2, 0x2000, 0);
							else if (col == 2)
								SFX_PlayX(SND_SELECTWHEEL, 0x2000, 0);

							timer = 10;
							speed = boxH[col] / 10;
							k = y[col][0] <= y[col][1];
							y[col][k] = boxY[col] - boxH[col];
							state = 2;
							idx[col][k] -= 2;

							if (idx[col][k] < 0)
								idx[col][k] += DAT_800d33c4[col];

							if (img[col][k])
								img[col][k]->unpack();

							img[col][k] = 0;
						} else if (press == 4) {
							if (col == 0)
								SFX_PlayX(SND_SELECTBOARD3, 0x2000, 0);
							else if (col == 1)
								SFX_PlayX(SND_SELECTTRUCK2, 0x2000, 0);
							else if (col == 2)
								SFX_PlayX(SND_SELECTWHEEL, 0x2000, 0);

							timer = 10;
							state = 2;
							speed = -boxH[col] / 10;
						}
					}

					break;
				case 1:
					if (--timer == 3) {
						col += dir;

						if (col < 0)
							col = 2;

						if (col >= 3)
							col = 0;

						savedTextState = textState;
						savedTextTimer = textTimer;
						textState = 3;
						textTimer = 2;
					}

					if (timer == 0) {
						state = 0;
						again = 1;
					}

					break;
				case 2:
					if (timer == 10 && col == 0) {
						textState = 4;
						textTimer = 1;
					}

					{
						int j;

						for (j = 0; j < 2; j++)
							y[col][j] += speed;
					}

					if (--timer == 0) {
						int k = y[col][0] >= y[col][1];

						if (speed < 0) {
							y[col][k] = boxY[col] + boxH[col];
							idx[col][k] += 2;

							if (idx[col][k] >= DAT_800d33c4[col])
								idx[col][k] -= DAT_800d33c4[col];

							if (img[col][k])
								img[col][k]->unpack();

							img[col][k] = 0;
							k = 1 - k;
							y[col][k] = boxY[col];

							if (col == 0)
								deck++;

							if (deck >= 8)
								deck = 0;

							if (col == 1)
								trucks++;

							if (trucks >= 3)
								trucks = 0;
						} else {
							y[col][k] = boxY[col];
							y[col][1 - k] = boxY[col] + boxH[col];

							if (col == 0)
								deck--;

							if (deck < 0)
								deck = 7;

							if (col == 1)
								trucks--;

							if (trucks < 0)
								trucks = 2;
						}

						again = 1;
						state = 0;

						if (col == 1)
							statsDone = 0;
					}

					break;
			}
		} while (again);

		switch (textState) {
			case 2:
				textTimer--;
				textX += textSpeed;

				if (textTimer == 0) {
					textState = 0;
					textTimer = 2;
				}

				break;
			case 3:
				if (--textTimer == 0) {
					textState = savedTextState;
					textTimer = savedTextTimer;
				}

				break;
			case 4:
				if (--textTimer == 0) {
					textSpeed = -textSpeed;
					textState = 5;
					textTimer = 10;
				}

				break;
			case 5:
				textTimer--;
				textX += textSpeed;

				if (textTimer == 0) {
					if (Career_Progress(DAT_800d22f0)->NumTapes >=
					    Stats[DAT_800d22f0].Boards[deck].Req) {
						strcpy(text, Stats[DAT_800d22f0].Boards[deck].Name);
					} else if (GGame == 1) {
						sprintf(text, "Get %d tapes to unlock",
						        Stats[DAT_800d22f0].Boards[deck].Req);
					} else {
						sprintf(text, "Unlock in career mode",
						        Stats[DAT_800d22f0].Boards[deck].Req);
					}

					Mess_SetCurrentFont("mainf.fnt");

					textX = -Mess_TextWidth(text);

					{
						int h = Mess_TextHeight(text);

						textY = (h >> 1) + 71 + Mess_TextHeightBelowBaseline(text);
					}

					textState = 2;
					textTimer = 10;
					textSpeed = (117 - textX) / 10;
					textX = 117 - textSpeed * 10;
				}

				break;
			case 0:
				if (--textTimer == 0) {
					textState = 1;
					statsDone = 0;
					statsTimer = 3;
				}

				break;
		}

		if (!statsDone) {
			board[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
			board[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
			board[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
			board[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];

			if (n < deck) {
				delta[0] = Stats[DAT_800d22f0].Boards[deck].Stats[0] - board[0];
				delta[1] = Stats[DAT_800d22f0].Boards[deck].Stats[1] - board[1];
				delta[2] = Stats[DAT_800d22f0].Boards[deck].Stats[2] - board[2];
				delta[3] = Stats[DAT_800d22f0].Boards[deck].Stats[3] - board[3];
			} else {
				delta[0] = 0;
				delta[1] = 0;
				delta[2] = 0;
				delta[3] = 0;
			}

			board[4] = DAT_800be438[trucks];

			if (--statsTimer == 0)
				statsDone = 1;
		}

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			result = 1;
			GBoardStats[0].s[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
			GBoardStats[0].s[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
			GBoardStats[0].s[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
			GBoardStats[0].s[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];
			GBoardStats[0].s[4] = DAT_800be438[trucks];

			if (Career_Progress(DAT_800d22f0)->NumTapes >=
			    Stats[DAT_800d22f0].Boards[deck].Req) {
				int j;

				/* Two nested one-pass loops (macro-like): they double the refs of the
				   y/idx temps so local-alloc gives the y[c][1] loads the short-lived
				   registers first, and the loop note before the call frees sched2. */
				do {
					do {
						SFX_PlayX(SND_SELECT1, 0x2000, 0);

						j = y[0][0] >= y[0][1];
						GCareer.LastDeck[0] = idx[0][j];
						j = y[1][0] >= y[1][1];
						GCareer.LastTrucks[0] = idx[1][j];
						j = y[2][0] >= y[2][1];
						GCareer.LastWheel[0] = idx[2][j];
					} while (0);
				} while (0);

				break;
			}

			SFX_PlayX(SND_NOSELECT1, 0x2000, 0);
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(SND_GOBACK2, 0x2000, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			result = -1;
			break;
		}
	}

	HiRes_BlankScreen();
	SpoolerBlocksFileIO->unloadPRE("Equip1.pre");

	delete input;
	delete textBar;

	for (i = 0; i < 3; i++) {
		delete box[i];
		delete title[i];
	}

	for (i = 0; i < 5; i++) {
		delete bar[i];
		delete row[i];
	}

	Front_FreeBoardPictures1P();
	FrontLoader_Defragment();
	Pack_DefragmentFully();

	return result;
}

/* Front_InitSkaterSpool2P__Fv (0x80055eb8) */
/* Credits state reset. The start position is the midpoint of two career values on a circular
   scale of 11, taking the shorter way round. */
/* PC: the skater the selects show after (dir 1) or before (dir -1) skater s: the game's ten,
   Officer Dick (10) once unlocked, then the mods' skaters. */
static int Front_StepSkater(int s, int dir, int secret) {
	do {
		s += dir;

		if (s >= GNumSkaters)
			s = 0;
		else if (s < 0)
			s = GNumSkaters - 1;
	} while (s == 10 && !secret);

	return s;
}

void Front_InitSkaterSpool2P(void) {
	int i;
	unsigned char hi, lo;

	DAT_800d231c[0] = 0;
	DAT_800d231c[1] = 0;

	for (i = 0; i < GNumSkaters; i++)
		DAT_800d3788[i] = 0;

	DAT_800d230c = -1;
	hi = GCareer.LastSkater[1] < GCareer.LastSkater[0] ? GCareer.LastSkater[0] : GCareer.LastSkater[1];
	lo = GCareer.LastSkater[1] < GCareer.LastSkater[0] ? GCareer.LastSkater[1] : GCareer.LastSkater[0];

	{
		int h1 = hi - GNumSkaters;

		if (lo - h1 < hi - lo) {
			int h2 = hi + GNumSkaters;

			DAT_800d2310 = (lo + h2) / 2;

			if (DAT_800d2310 >= GNumSkaters)
				DAT_800d2310 -= GNumSkaters;
		} else {
			DAT_800d2310 = (lo + hi) / 2;
		}
	}

	DAT_800d2314 = 0;
}

/* Front_SpoolNextSkater2P__Fv (0x80055f80) */
void Front_SpoolNextSkater2P() {
	int a;
	int n;

	FileIO_Continue();
	Spool_Continue();

	if (FUN_80070830())
		return;

	Spool_Sync();

	a = DAT_800d230c;

	if (a >= 0)
		DAT_800d3788[DAT_800d22f4] = 1;

	if (a == -1) {
		DAT_800d230c = 0;
	} else {
		/* out from the middle skater: 0, -1, +1, -2, ... */
		if (a < GNumSkaters / 2)
			n = GNumSkaters - 1;
		else if (a == GNumSkaters / 2) {
			DAT_800d2314 = 1;
			return;
		} else
			n = GNumSkaters;

		DAT_800d230c = n - a;
	}

	DAT_800d22f4 = DAT_800d230c + DAT_800d2310;

	if (!(DAT_800d22f4 < GNumSkaters))
		DAT_800d22f4 = DAT_800d22f4 - GNumSkaters;

	DAT_800d3758[DAT_800d22f4] = Spool_PSX(Stats[DAT_800d22f4].FrontName, 2, 1);
}

/* Front_FreeSkaterSpool2P__Fv (0x8005609c) */
void Front_FreeSkaterSpool2P() {
	int i, v, j;

	Front_SelectSkaterPack(0);

	for (i = 0; i < 2; i++) {
		if (DAT_800d231c[i]) {
			ClearRegion(DAT_800d3758[DAT_800d2304[i]], 1, 1);
			Spool_RestorePSX(DAT_800d3758[DAT_800d2304[i]]);

			DAT_800d231c[i] = 0;

			if (DAT_800d2304[0] == DAT_800d2304[1])
				DAT_800d231c[1 - i] = 0;
		}
	}

	Spool_Sync();

	v = DAT_800d230c + DAT_800d2310;

	if (v >= GNumSkaters)
		v -= GNumSkaters;

	j = 0;

	if (DAT_800d2314 == 0)
		DAT_800d3788[v] = 1;

	for (j = 0; j < GNumSkaters; j++) {
		if (DAT_800d3788[j]) {
			ProcessNewPSX(DAT_800d3758[j]);
			Spool_ClearPSX(Stats[j].FrontName);
		}
	}

	Spool_Sync();
}

/* Front_CreateSkaterModel2P__Fi (0x8005622c) */
/* Two-player sibling of Front_CreateSkaterModel1P: new CBruce for player Player with the
   skater index from DAT_800d2304[Player], a per-player x position, and the stance flag. Lever:
   -fstructure-noalias (the index re-reads come from the separate array expressions). */
CBruce *Front_CreateSkaterModel2P(int Player) {
	CBruce *pBruce = new CBruce;

	pBruce->mSkaterIndex = DAT_800d2304[Player];

	pBruce->InitItem(Stats[DAT_800d2304[Player]].FrontName);

	pBruce->mAnimSpeed = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->mRGB = 0x404040;

	if (!Player)
		pBruce->mPos.Set(DAT_800d2300, DAT_800d22f8, 0x1bc000);
	else
		pBruce->mPos.Set(DAT_800d22fc, DAT_800d22f8, 0x1bc000);

	M3dMaths_SetIdentityRotation(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d2304[Player]].pBio[4] == 'R' || *Stats[DAT_800d2304[Player]].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;

	pBruce->mpLight = (SLight *)DAT_800be444;
	return pBruce;
}

/* Front_ResetDrawArea2P__Fv (0x80056394) */
void Front_ResetDrawArea2P(void) {
	DR_AREA *p = (DR_AREA *)pPoly;
	RECT r;

	setRECT(&r, 0, 0, 640, 480);
	SetDrawArea(p, &r);

	addPrim(&pDoubleBuffer->OT3D[4], p);

	pPoly = (u_char *)(p + 1);
}

/* Front_UnloadSkaterTextures2P__Fv (0x80056428) */
void Front_UnloadSkaterTextures2P(void) {
	int i, n;
	TSPack *t;

	Front_SelectSkaterPack(0);

	n = 2;

	if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[0] && DAT_800d231c[1])
		n = 1;

	for (i = 0; i < n; i++) {
		if (DAT_800d231c[i]) {
			ClearRegion(DAT_800d3758[DAT_800d2304[i]], 1, 1);
			Spool_RestorePSX(DAT_800d3758[DAT_800d2304[i]]);
		}
	}

	if (Pack_pFree) {
		do {
			t = Pack_pFree->pNext;
			Pack_KillSlot(Pack_pFree);

			Pack_pFree = t;
		} while (t);
	}

	Pack_pFree = (TSPack *)Mem_NewX(sizeof(TSPack), 1, 1); /* (24 bytes in 64-bit) */
	*Pack_pFree = *DAT_800d1c80;
	Pack_pFree->pNext = 0;
}

/* Front_LoadSkaterTextures2P__Fv (0x800565ac) */
void Front_LoadSkaterTextures2P() {
	short ab[4];
	SStats *e;
	int i, n;

	Front_SelectSkaterPack(0);

	n = 2;

	if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[0] && DAT_800d231c[1])
		n = 1;

	for (i = 0; i < n; i++) {
		if (n == 2) {
			if (i == 0)
				Pack_Pack(0, 128, 256, (int *)&ab[0], (int *)&ab[2], 16, 0, 0);
			else
				Pack_Unpack(ab[0], ab[2]);
		}

		if (DAT_800d231c[i]) {
			Spool_BackupPSX(DAT_800d3758[DAT_800d2304[i]]);

			if (Cheat_BigHead) {
				e = Stats + DAT_800d2304[i];
				GHeadUp = e->Neck;
				GHeadPart = e->Head;
			}

			ProcessNewPSX(DAT_800d3758[DAT_800d2304[i]]);

			GHeadPart = -1;
		}
	}
}

/* Front_SkaterSelect2P__Fv (0x80056754) */
/* C++ (front end, 2-player skater select). Each player spins a CBruce model of the highlighted
   skater on a turntable inside a frame; left/right slide the model out and the next skater in
   (models stream in through the credits spooler, "LOADING" is shown until the PSX is resident),
   the skater's name slides in on a text bar, and after the name settles an info panel (six text
   lines) and four stat bars are drawn. X/Start locks a skater in ("skater chosen"), triangle
   backs out. Returns player 1's skater (player 2's goes to SelectedSkater2), or -1. No demo twin.
   Levers (as in the 1P sibling): the function-wide `i` is reused as the counter of the stat
   bars' inner loop and of the teardown loops, which lowers its global-alloc priority so the
   counters land in s1 under the pointers/offsets; the best-board scan is a top-tested
   `while (best < 7) { if (tapes < Req) break; best++; }`. */
int Front_SkaterSelect2P(void) {
	char spare[40]; /* never referenced; retail's frame has these 40 bytes at sp+0x20 */
	char name[16];
	Sprite *bars[4];
	Sprite *buttons[4];
	int leftX[2];
	int rightX[2];
	int leftTimer[2];
	int rightTimer[2];
	SStatBar stat[2][4];
	int best[2];
	Sprite *info[3][2];
	int panelX[2];
	int panelY[2];
	Sprite *panel[2];
	Sprite *textBar[2];
	Sprite *frame[2];
	int minX[2];
	int maxX[2];
	int homeX[2];
	SVECTOR rot[2];
	CBruce *bruce[2];
	int state[2];
	int speed[2];
	int shown[2];
	int result;
	SViewport *vp;
	int i;
	int p;
	void *buf;
	void *bmp;

	FrontLoader_SetFiles(2);
	FrontLoader_LoadSet(2);
	FrontLoader_Defragment();

	buf = Mem_NewX(0x26160, 1, 1);
	bmp = FrontLoader_TakeFile("select2p.bmp");

	Mem_Copy(buf, bmp, 0x26160);
	Mem_Delete(bmp);
	Mess_LoadFont("stats2.fnt", -1, -1, -1);
	Mess_LoadFont("mainf.fnt", -1, -1, -1);
	Mess_SetFontScale(200);
	DownloadBMP16x2(buf);

	result = 0;
	DAT_800d1c20 = 0;

	for (i = 0; i < 11; i++) {
		if (Career_Progress(i)->NumTapes >= 30) {
			DAT_800d1c20 = 1;
		}
	}

	for (int k = 0; k < 2; k++) {
		DAT_800d2304[k] = GCareer.LastSkater[k];

		if (!DAT_800d1c24 && DAT_800d1c20 && DAT_800d2304[1 - k] != 10)
			DAT_800d2304[k] = 10;

		if ((DAT_800d2304[k] == 10 && !DAT_800d1c20) || DAT_800d2304[k] >= GNumSkaters)
			DAT_800d2304[k] = k;
	}

	DAT_800d1c24 = DAT_800d1c20;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	if (DAT_800d2304[0] == DAT_800d2304[1])
		DAT_800d2304[1] = Front_StepSkater(DAT_800d2304[0], 1, DAT_800d1c20);

	Pack_DefragmentFully();
	Front_InitSkaterPack(2);

	for (i = 0; i < 4; i++) {
		sprintf(name, "bar%d.bmp", i + 5);

		bars[i] = new Sprite(name, 1, 32, 128);
		bars[i]->removeFromMemory();
	}

	for (i = 0; i < 4; i++) {
		sprintf(name, "levbut%d.bmp", i + 1);

		buttons[i] = new Sprite(name, 1, 32, 128);
		buttons[i]->removeFromMemory();
	}

	leftX[0] = 98;
	leftX[1] = 386;
	rightX[0] = 271;
	rightX[1] = 559;
	leftTimer[0] = leftTimer[1] = 0;
	rightTimer[0] = rightTimer[1] = 0;

	for (int p = 0; p < 2; p++) {
		int k;

		for (k = 0; k < 4; k++) {
			if (p == 0)
				stat[p][k].x = 203;
			else
				stat[p][k].x = 476;

			stat[p][k].y = k * 17 + 256;
			stat[p][k].type = k;
		}

		stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
		stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
		stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
		stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
	}

	info[0][0] = new Sprite(203, 323, 150, 30, 1, 32, 128);

	info[0][0]->removeFromMemory();

	info[0][1] = info[0][0];
	info[1][0] = new Sprite(203, 352, 165, 18, 1, 32, 128);

	info[1][0]->removeFromMemory();

	info[1][1] = info[1][0];
	info[2][0] = new Sprite(203, 369, 90, 47, 1, 32, 128);

	info[2][0]->removeFromMemory();

	info[2][1] = info[2][0];

	for (int p = 0; p < 2; p++) {
		if (p == 0)
			panelX[p] = 203;
		else
			panelX[p] = 476;

		panelY[p] = 248;
	}

	panel[0] = new Sprite(panelX[0], panelY[0], 160, 90, 1, 32, 128);

	panel[0]->removeFromMemory();

	panel[1] = panel[0];
	textBar[0] = new Sprite(0, 50, 340, 41, 1, 32, 128);

	textBar[0]->removeFromMemory();

	textBar[1] = new Sprite(340, 50, 300, 41, 1, 32, 128);

	textBar[1]->removeFromMemory();

	frame[0] = new Sprite(144, 92, 129, 142, 1, 16, 32);

	frame[0]->removeFromMemory();

	frame[1] = frame[0];

	minX[0] = DAT_800d2300 - DAT_800d1c1c * 0x7000;
	minX[1] = DAT_800d22fc - DAT_800d1c1c * 0x7000;
	maxX[0] = DAT_800d2300 + DAT_800d1c1c * 0x7000;
	maxX[1] = DAT_800d22fc + DAT_800d1c1c * 0x7000;
	homeX[0] = DAT_800d2300;
	homeX[1] = DAT_800d22fc;

	DrawSync(0);
	Front_InitSkaterSpool2P();
	DrawSync(0);

	rot[0].vx = rot[0].vy = rot[0].vz = 0;
	rot[1].vx = rot[1].vy = rot[1].vz = 0;
	vp = HiRes_SetupCamera();
	vp->xR = 640;
	vp->yB = 480;
	bruce[0] = 0;
	bruce[1] = 0;

	for (int j = 0; j < 2; j++) {
		state[j] = 0;
		speed[j] = DAT_800d1c1c;
		shown[i] = 1; /* sic: i (== 4) from the sprite loop, not j */
	}

	SLabel7 sSkater = "skater";
	int pad1f0[2]; /* unreferenced 8-byte gaps around the labels in retail's frame */
	SLabel7 sChosen = "chosen";
	int pad200[2];
	int lx[2];
	int ly[2];
	PadBuffer *input[2];
	int panelState[2];
	int infoTimer[2];
	int unused[2];
	int infoX[2];
	int frameX[2];
	int barX[2];
	int textX[2];
	int textY[2];
	int startX[2];
	int endX[2];
	int textSpeed[2];
	int textState[2];
	int loadX[2];

	lx[0] = 208;
	lx[1] = 496;
	ly[0] = 153;
	ly[1] = 173;
	input[0] = new PadBuffer(0);
	input[1] = new PadBuffer(1);

	for (int j = 0; j < 2; j++) {
		panelState[j] = 0;
		infoTimer[j] = 0;
		unused[j] = 0;
	}

	infoX[0] = 203;
	infoX[1] = 476;
	frameX[0] = 144;
	frameX[1] = 432;
	barX[0] = 0;
	barX[1] = 340;
	textState[0] = 0;
	textState[1] = 0;
	pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? &DoubleBuffer[1] : DoubleBuffer;

	HiRes_ClearOTag();

	pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? &DoubleBuffer[1] : DoubleBuffer;

	HiRes_ClearOTag();

	loadX[0] = frameX[0] + 64;
	loadX[1] = frameX[1] + 64;

	FrontLoader_SetFiles(10);

	for (;;) {
		Front_UpdateMusicLoop();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			goto out;

		PollHost();

		if (!DAT_800d2314)
			Front_SpoolNextSkater2P();
		else
			FrontLoader_Continue();

		HiRes_ClearOTag();
		ResetOT();

		for (p = 0; p < 2; p++) {
			int done;
			int press;

			Front_SelectSkaterPack(0);
			input[p]->update();

			if (DAT_800d231c[p]) {
				bruce[p]->UpdateFrame();
				bruce[p]->UpdateFrame();
			}

			done = 0;

			switch (state[p]) {
				case 0:
				default:
					rot[p].vy += 10;
					press = input[p]->getPress();

					if (press == 2) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state[p] = 2;
						textState[p] = 3;
						rightTimer[p] = 16;
					} else if (press == 1) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state[p] = 3;
						textState[p] = 3;
						leftTimer[p] = 16;
					}

					if (!DAT_800d231c[p] && DAT_800d3788[DAT_800d2304[p]]) {
						if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[1 - p]) {
							DAT_800d231c[p] = 1;
						} else {
							DAT_800d231c[p] = 1;
							Spool_BackupPSX(DAT_800d3758[DAT_800d2304[p]]);

							if (Cheat_BigHead) {
								GHeadUp = Stats[DAT_800d2304[p]].Neck;
								GHeadPart = Stats[DAT_800d2304[p]].Head;
							}

							ProcessNewPSX(DAT_800d3758[DAT_800d2304[p]]);

							GHeadPart = -1;
						}

						bruce[p] = Front_CreateSkaterModel2P(p);
						bruce[p]->mPos.vx = homeX[p];
					}

					break;
				case 1:
					input[p]->reset();

					if (DAT_800d231c[p]) {
						while (rot[p].vy > 2048)
							rot[p].vy -= 4096;

						while (rot[p].vy < -2048)
							rot[p].vy += 4096;

						if (rot[p].vy > -80 && rot[p].vy < 80)
							rot[p].vy = 0;
						else if (rot[p].vy < 0)
							rot[p].vy += 80;
						else if (rot[p].vy > 0)
							rot[p].vy -= 80;

						if (bruce[p]->mPos.vx > homeX[p] - (DAT_800d1c1c << 13) &&
						    bruce[p]->mPos.vx < homeX[p] + (DAT_800d1c1c << 13))
							bruce[p]->mPos.vx = homeX[p];
						else if (bruce[p]->mPos.vx < homeX[p])
							bruce[p]->mPos.vx += DAT_800d1c1c << 13;
						else if (bruce[p]->mPos.vx > homeX[p])
							bruce[p]->mPos.vx -= DAT_800d1c1c << 13;
					}

					break;
				case 2:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx -= speed[p] << 12;

						if (bruce[p]->mPos.vx < minX[p])
							state[p] = 5;
					} else {
						loadX[p] -= 12;

						if (loadX[p] < frameX[p])
							state[p] = 5;
					}

					break;
				case 3:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx += speed[p] << 12;

						if (bruce[p]->mPos.vx >= maxX[p])
							state[p] = 4;
					} else {
						loadX[p] += 12;

						if (loadX[p] >= frameX[p] + 129)
							state[p] = 4;
					}

					break;
				case 5:
					shown[1 - p] = 0;
					state[p] = 7;
					break;
				case 7:
					state[p] = 9;
					Front_UnloadSkaterTextures2P();

					if (DAT_800d231c[p]) {
						if (bruce[p]) {
							delete bruce[p];
							bruce[p] = 0;
						}
					}

					DAT_800d2304[p] = Front_StepSkater(DAT_800d2304[p], 1, DAT_800d1c20);

					DAT_800d231c[p] = DAT_800d3788[DAT_800d2304[p]];
					Front_LoadSkaterTextures2P();

					if (DAT_800d231c[p]) {
						if (bruce[p])
							printf("\nOH FUCK!!!");

						bruce[p] = Front_CreateSkaterModel2P(p);
						bruce[p]->mPos.vx = maxX[p];
					}

					loadX[p] = frameX[p] + 129;
					textState[p] = 0;
					stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
					stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
					stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
					stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
					break;
				case 4:
					shown[1 - p] = 0;
					state[p] = 6;
					break;
				case 6:
					state[p] = 8;
					Front_UnloadSkaterTextures2P();

					if (DAT_800d231c[p]) {
						if (bruce[p]) {
							delete bruce[p];
							bruce[p] = 0;
						}
					}

					DAT_800d2304[p] = Front_StepSkater(DAT_800d2304[p], -1, DAT_800d1c20);

					DAT_800d231c[p] = DAT_800d3788[DAT_800d2304[p]];
					Front_LoadSkaterTextures2P();

					if (DAT_800d231c[p]) {
						if (bruce[p])
							printf("\nOH FUCK!!!");

						bruce[p] = Front_CreateSkaterModel2P(p);
						bruce[p]->mPos.vx = minX[p];
					}

					loadX[p] = frameX[p];
					textState[p] = 0;
					stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
					stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
					stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
					stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
					break;
				case 8:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx += speed[p] << 12;

						if (bruce[p]->mPos.vx >= homeX[p]) {
							done = 1;
							bruce[p]->mPos.vx = homeX[p];
						}
					} else {
						loadX[p] += 12;

						if (loadX[p] >= frameX[p] + 64) {
							loadX[p] = frameX[p] + 64;
							done = 1;
						}
					}

					if (done) {
						state[p] = 0;
						press = input[p]->getPress();

						if (press == 2) {
							SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

							state[p] = 2;
							textState[p] = 3;
							rightTimer[p] = 16;
						} else if (press == 1) {
							SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

							state[p] = 3;
							textState[p] = 3;
							leftTimer[p] = 16;
						}
					}

					break;
				case 9:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx -= speed[p] << 12;

						if (bruce[p]->mPos.vx <= homeX[p]) {
							bruce[p]->mPos.vx = homeX[p];
							done = 1;
						}
					} else {
						loadX[p] -= 12;

						if (loadX[p] <= frameX[p] + 64) {
							loadX[p] = frameX[p] + 64;
							done = 1;
						}
					}

					if (done) {
						state[p] = 0;
						press = input[p]->getPress();

						if (press == 2) {
							SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

							state[p] = 2;
							textState[p] = 3;
							rightTimer[p] = 16;
						} else if (press == 1) {
							SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

							state[p] = 3;
							textState[p] = 3;
							leftTimer[p] = 16;
						}
					}

					break;
			}

			SetOTPlayer(p);

			if (DAT_800d231c[p]) {
				RotMatrixYXZ(&rot[p], &bruce[p]->mRotMatrix);

				bruce[p]->mRotMatrix.m[0][1] = -bruce[p]->mRotMatrix.m[0][1];
				bruce[p]->mRotMatrix.m[1][1] = -bruce[p]->mRotMatrix.m[1][1];
				bruce[p]->mRotMatrix.m[2][1] = -bruce[p]->mRotMatrix.m[2][1];

				if (shown[p])
					frame[p]->draw(frameX[p], 92, 0, -1, -1, -1);

				if (bruce[p]->mPos.vx >= minX[p] && maxX[p] >= bruce[p]->mPos.vx && shown[p]) {
					bruce[p ^ 1]->mFlags ^= 0x41;
					M3d_BuildTransform(bruce[p]);

					bruce[0]->mShadowLights = 0;
					bruce[1]->mShadowLights = 0;
					M3d_RenderSetup((SCamera *)MikeCamera, (SViewport *)vp,
					                &pDoubleBuffer->OT3D[p << 12]);
					M3d_Render(MechList);
					M3d_RenderCleanup();

					bruce[p ^ 1]->mFlags ^= 0x41;
				} else if (!shown[p]) {
					shown[p] = 1;
				}
			} else if (state[p] == 0) {
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(loadX[p], 163, "LOADING", 0, 0, 256, 0);
			}

			if (state[p] == 1) {
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(250, 190, 60, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(lx[p], ly[0], sSkater, 1, 0, 256, 0);
				Mess_DrawText(lx[p], ly[1], sChosen, 1, 0, 256, 0);
			}

			SetOTPlayer(p);

			if (infoTimer[p]) {
				infoTimer[p]--;
				info[0][p]->draw(infoX[p], 323, 0, -1, -1, -1);
				info[1][p]->draw(infoX[p], 352, 0, -1, -1, -1);
				info[2][p]->draw(infoX[p], 369, 0, -1, -1, -1);
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(1);
				int ty = 330;

				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[0]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[0]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[0], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[1]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[1]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[1], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[2]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[2]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[2], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[3]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[3]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[3], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[4]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[4]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[4], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText(infoX[p],
				              Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].pBio[5]) + ty -
				                  (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[5]) >> 1),
				              Stats[DAT_800d2304[p]].pBio[5], 0, 0, 256, 0);
			}

			if (panelState[p] == -1 && panelState[1 - p] <= 0 && infoTimer[1 - p] <= 0 &&
			    textState[1 - p] == 1) {
				panelState[p] = 2;
			} else if (panelState[p] > 0) {
				int k;

				if (--panelState[p] == 0)
					infoTimer[p] = 2;

				panel[p]->draw(panelX[p], panelY[p], 0, -1, -1, -1);

				best[p] = 0;

				while (best[p] < 7) {
					if (Career_Progress(DAT_800d2304[p])->NumTapes <
					    Stats[DAT_800d2304[p]].Boards[best[p] + 1].Req)
						break;

					best[p]++;
				}

				for (k = 0; k < 4; k++) {
					int x = stat[p][k].x;
					int type = stat[p][k].type;
					int y = stat[p][k].y - (bars[type]->m_h >> 1);

					for (i = 0; i < stat[p][k].value; i++) {
						bars[type]->draw(x, y, 0, -1, -1, -1);

						x += 14;
					}
				}
			}

			if (leftTimer[p]) {
				if (--leftTimer[p] == 0)
					buttons[0]->draw(leftX[p], 95, 0, -1, -1, -1);
				else
					buttons[2]->draw(leftX[p], 95, 0, -1, -1, -1);
			}

			if (rightTimer[p]) {
				if (--rightTimer[p] == 0)
					buttons[1]->draw(rightX[p], 95, 0, -1, -1, -1);
				else
					buttons[3]->draw(rightX[p], 95, 0, -1, -1, -1);
			}

			switch (textState[p]) {
				case 1:
					break;
				case 0:
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify(1);

					if (p == 0) {
						startX[p] = textX[p] = -Mess_TextWidth(Stats[DAT_800d2304[p]].Name);
						endX[p] = 208 - (Mess_TextWidth(Stats[DAT_800d2304[p]].Name) >> 1);
					} else {
						startX[p] = textX[p] = 640;
						endX[p] = 496 - (Mess_TextWidth(Stats[DAT_800d2304[p]].Name) >> 1);
					}

					textSpeed[p] = (endX[p] - startX[p]) / 6;
					startX[p] -= textSpeed[p];
					endX[p] -= textSpeed[p];

					{
						int ty = 74;

						textY[p] = Mess_TextHeightAboveBaseline(Stats[DAT_800d2304[p]].Name) + ty -
						           (Mess_TextHeight(Stats[DAT_800d2304[p]].Name) >> 1);
					}

					textState[p] = 2;
					break;
				case 2:
					textX[p] += textSpeed[p];

					if ((textX[p] < startX[p] && textX[p] < endX[p]) ||
					    (textX[p] > startX[p] && textX[p] > endX[p])) {
						panelState[p] = -1;
						textState[p] = 1;
					}

					textBar[p]->draw(barX[p], 50, 0, -1, -1, -1);
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify(1);
					Mess_DrawText(textX[p], textY[p], Stats[DAT_800d2304[p]].Name, 0, 0, 256, 0);
					break;
				case 3:
					textX[p] -= textSpeed[p];

					if ((textX[p] < startX[p] && textX[p] < endX[p]) ||
					    (textX[p] > startX[p] && textX[p] > endX[p]))
						textState[p] = 1;

					textBar[p]->draw(barX[p], 50, 0, -1, -1, -1);
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify(1);
					Mess_DrawText(textX[p], textY[p], Stats[DAT_800d2304[p]].Name, 0, 0, 256, 0);
					break;
			}

			SetOTPlayer(p);

			if (DAT_800d231c[p])
				SetClippingBox(frameX[p], 92, 129, 142);
			else
				SetClippingBox(0, 0, 640, 480);

			if (p == 0) {
				FlickerSync();

				pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? &DoubleBuffer[1] : DoubleBuffer;
				PC_DrawOT(&pDoubleBuffer->OT3D[0x1fff]);

				pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? &DoubleBuffer[1] : DoubleBuffer;
			}
		}

		Front_ResetDrawArea2P();
		FlickerSync();
		HiRes_DrawOTagAll();

		for (p = 0; p < 2; p++) {
			if (Pad[p].MenuStart.Triggered || Pad[p].MenuAccept.Triggered) {
				Pad[p].MenuStart.Triggered = 0;
				Pad[p].MenuAccept.Triggered = 0;

				if (state[p] != 1 && DAT_800d231c[p] &&
				    (DAT_800d2304[0] != DAT_800d2304[1] || state[1 - p] != 1)) {
					state[p] = 1;
					SFX_PlayX(SND_SELECT1, 0x2000, 0);
				} else if (state[p] != 1 && DAT_800d231c[p]) {
					SFX_PlayX(SND_NOSELECT1, 0x2000, 0);
				}
			}

			if (Pad[p].MenuBack.Triggered) {
				Pad[p].MenuBack.Triggered = 0;

				if (state[p] != 1) {
					if (state[1 - p] != 1) {
						SFX_PlayX(SND_GOBACK2, 0x2000, 0);

						result = -1;
						break;
					}
				} else {
					SFX_PlayX(SND_GOBACK2, 0x2000, 0);

					state[p] = 0;
				}
			}
		}

		if (result == -1)
			goto out;

		if (state[0] == 1 && state[1] == 1) {
			result = DAT_800d2304[0];

			for (int k = 0; k < 2; k++) {
				if (GCareer.LastSkater[k] != DAT_800d2304[k]) {
					GCareer.LastSkater[k] = 0;
					GCareer.LastDeck[k] = 0;
					GCareer.LastWheel[k] = 0;
					GCareer.LastTrucks[k] = 1;
					GCareer.LastLevel = 0;
				}

				GCareer.LastSkater[k] = DAT_800d2304[k];
			}

			goto out;
		}
	}

out:

	HiRes_BlankScreen();

	if (Loading)
		FileIO_Sync();

	for (i = 0; i < 2; i++) {
		if (DAT_800d231c[i]) {
			delete bruce[i];
			bruce[i] = 0;
		}
	}

	Front_FreeSkaterSpool2P();
	Front_FreeSkaterPack();

	for (i = 0; i < 4; i++)
		delete bars[i];

	for (int k = 0; k < 4; k++)
		delete buttons[k];

	delete info[0][0];
	delete info[1][0];
	delete info[2][0];
	delete panel[0];
	delete frame[0];

	for (i = 0; i < 2; i++) {
		delete textBar[i];
		delete input[i];
	}

	SelectedSkater2 = DAT_800d2304[1];
	return result;
}

/* FUN_80058fc8 (0x80058fc8) */
void FUN_80058fc8(void) {
	DAT_800d2300 = -(DAT_800d1c0c * 112);
	DAT_800d22fc = DAT_800d1c0c * 176;
	DAT_800d22f8 = (DAT_800d1c18 - 77) * DAT_800d1c10;
}

/* FlickerSync__Fv (0x80059018) */
void FlickerSync(void) {
	PC_FrameWait(1);

	/* the interlace field (libgpu's GetODE): odd and even frames */
	for (;;) {
		PC_FramePoll();

		if ((int)(PC_FrameCount() & 1) != DAT_800d1c44)
			break;
	}

	DAT_800d1c44 ^= 1;
}

/* Front_InitSkaterSpool1P__Fv (0x80059050) */
void Front_InitSkaterSpool1P(void) {
	int i;

	DAT_800d2338 = -1;
	DAT_800d2340 = 0;

	for (i = 0; i < GNumSkaters; i++)
		DAT_800d37e4[i] = 0;

	DAT_800d2344 = 0;
	DAT_800d233c = GCareer.LastSkater[0];
}

/* Front_SpoolNextSkater1P__Fv (0x80059094) */
void Front_SpoolNextSkater1P() {
	int a;
	int n;

	FileIO_Continue();
	Spool_Continue();

	if (FUN_80070830())
		return;

	Spool_Sync();

	a = DAT_800d2338;

	if (a >= 0)
		DAT_800d37e4[DAT_800d2324] = 1;

	if (a == -1) {
		DAT_800d2338 = 0;
	} else {
		/* out from the middle skater: 0, -1, +1, -2, ... */
		if (a < GNumSkaters / 2)
			n = GNumSkaters - 1;
		else if (a == GNumSkaters / 2) {
			DAT_800d2340 = 1;
			return;
		} else
			n = GNumSkaters;

		DAT_800d2338 = n - a;
	}

	DAT_800d2324 = DAT_800d2338 + DAT_800d233c;

	if (!(DAT_800d2324 < GNumSkaters))
		DAT_800d2324 = DAT_800d2324 - GNumSkaters;

	if (*Stats[DAT_800d2324].FrontName)
		DAT_800d37b4[DAT_800d2324] = Spool_PSX(Stats[DAT_800d2324].FrontName, 2, 1);
}

/* Front_FreeSkaterSpool1P__Fv (0x800591c0) */
void Front_FreeSkaterSpool1P() {
	int i, v;

	if (DAT_800d2344 && *Stats[DAT_800d1c40].FrontName) {
		ClearRegion(DAT_800d37b4[DAT_800d1c40], 1, 1);
		Spool_RestorePSX(DAT_800d37b4[DAT_800d1c40]);
	}

	Spool_Sync();

	v = DAT_800d2338 + DAT_800d233c;

	if (v >= GNumSkaters)
		v -= GNumSkaters;

	i = 0;

	if (DAT_800d2340 == 0)
		DAT_800d37e4[v] = 1;

	for (i = 0; i < GNumSkaters; i++) {
		if (*Stats[i].FrontName && DAT_800d37e4[i]) {
			Front_ResetSkaterPack();
			ProcessNewPSX(DAT_800d37b4[i]);
			Spool_ClearPSX(Stats[i].FrontName);
		}
	}

	Spool_Sync();
}

/* Front_CreateSkaterModel1P__Fv (0x8005933c) */
/* Creates the skater shown in the credits: new CBruce, model of the selected skater
   (or skater 0 if that slot has no name), idle anim, fixed position, identity rotation, and the
   goofy flag from the stance string ('R'/'r'). Lever: -fstructure-noalias keeps the selected index
   in a register across the mSkaterIndex store. */
CBruce *Front_CreateSkaterModel1P() {
	CBruce *pBruce = new CBruce;

	if (!*Stats[DAT_800d1c40].FrontName) {
		pBruce->mSkaterIndex = 0;
		pBruce->InitItem(Stats[0].FrontName);
	} else {
		pBruce->mSkaterIndex = DAT_800d1c40;
		pBruce->InitItem(Stats[DAT_800d1c40].FrontName);
	}

	pBruce->mAnimSpeed = 0x10000;

	pBruce->CycleAnim(0, 1);

	pBruce->mRGB = 0x404040;

	pBruce->mPos.Set(DAT_800d232c, DAT_800d2328, 0xe6000);
	M3dMaths_SetIdentityRotation(&pBruce->mRotMatrix);

	if (*Stats[DAT_800d1c40].pBio[4] == 'R' || *Stats[DAT_800d1c40].pBio[4] == 'r')
		pBruce->mExtraFlags |= 2;

	pBruce->mpLight = (SLight *)DAT_800be444;
	return pBruce;
}

/* Front_ResetDrawArea1P__Fv (0x8005948c) */
void Front_ResetDrawArea1P(void) {
	DR_AREA *p = (DR_AREA *)pPoly;
	RECT r;

	setRECT(&r, 0, 0, 640, 480);
	SetDrawArea(p, &r);

	addPrim(&pDoubleBuffer->OT3D[4], p);

	pPoly = (u_char *)(p + 1);
}

/* Front_InitSkaterPack__Fi (0x80059520) */
/* TSPack (game/m3d.h) */
void Front_InitSkaterPack(int m) {
	TSPack *s;
	int t1, t2;
	int w = 256;

	if (m == 1)
		w = 128;

	s = Pack_Pack(-1, w, 256, &DAT_800d1c94, &DAT_800d1c98, 16, 0, 0);
	DAT_800d1c84 = (TSPack *)Mem_NewX(sizeof(TSPack), 1, 1);
	*DAT_800d1c84 = *s;
	DAT_800d1c80 = (TSPack *)Mem_NewX(sizeof(TSPack), 1, 1);
	*DAT_800d1c80 = *s;
	DAT_800d1c88 = 0;
	DAT_800d1c84->pNext = 0;
	DAT_800d1c8c = Pack_pFree;
	DAT_800d1c90 = Pack_pFull;
}

/* Front_SelectSkaterPack__Fi (0x80059618) */
void Front_SelectSkaterPack(int a) {
	if (DAT_800d1c5c == -1) {
		DAT_800d1c8c = Pack_pFree;
		DAT_800d1c90 = Pack_pFull;
	} else {
		DAT_800d1c84 = Pack_pFree;
		DAT_800d1c88 = Pack_pFull;
	}

	DAT_800d1c5c = a;

	if (a == -1) {
		Pack_pFree = DAT_800d1c8c;
		Pack_pFull = DAT_800d1c90;
	} else {
		Pack_pFree = DAT_800d1c84;
		Pack_pFull = DAT_800d1c88;
	}
}

/* Front_ResetSkaterPack__Fv (0x800596a0) */
/* TSPack (game/m3d.h) */
void Front_ResetSkaterPack(void) {
	TSPack *n;

	Front_SelectSkaterPack(0);

	if (Pack_pFree) {
		do {
			n = Pack_pFree->pNext;
			Mem_Delete(Pack_pFree);

			Pack_pFree = n;
		} while (n);
	}

	Pack_pFree = (TSPack *)Mem_NewX(sizeof(TSPack), 1, 1);
	*Pack_pFree = *DAT_800d1c80;
	Pack_pFree->pNext = 0;
	Front_SelectSkaterPack(0);
}

/* Front_FreeSkaterPack__Fv (0x80059758) */
void Front_FreeSkaterPack(void) {
	Front_SelectSkaterPack(-1);
	Mem_Delete(DAT_800d1c80);
	Pack_Unpack(*(short *)&DAT_800d1c94, *(short *)&DAT_800d1c98);
}

/* Front_SkaterSelect1P__Fv (0x80059794) */
/* C++ (front end, 1-player skater select; called from the screen dispatcher Front_Main). The
   selected skater (DAT_800d1c40) spins as a 3D CBruce in the right-hand window and slides out/in
   when left/right is pressed (or a "LOADING" label slides while the model streams in). The name
   slides in under a bar, four stat rows draw bar sprites (skater stats plus the best unlocked
   board's), the six bio lines are printed, and the career's tapes and the three competition medals
   (Skate Park, Burnside, Roswell) are shown in an item box. The secret 11th skater unlocks once any
   skater has 30 tapes. X/Start picks the skater (returns its index), triangle backs out (-1).
   size=9180 covers the whole TU tail: this function, Front_CalcSkaterPreviewBounds, and the out-of-line copy of
   CItem's inline virtual destructor (FUN_8005bb3c). No #pragma interface, so _vt.5CItem lands in
   .rdata before the jump tables, as in retail. Levers: the main loop is a for(;;) whose exits are
   all `goto done` (a `break` lets expand_end_loop roll the leading exit test to the bottom), and
   RotMatrixYXZ (RotMatrix) returns MATRIX*. */
int Front_SkaterSelect1P() {
	char text[40]; /* unused */
	char name[16];
	Sprite *bar[5];
	SStatRow rows[4];
	int bonus[4];
	Sprite *medal[3];
	int medalType[3];
	int i;

	int result = 0;

	DAT_800d1c60 = 0;
	DAT_800d1c40 = GCareer.LastSkater[0];

	for (i = 0; i < 11; i++) {
		if (Career_Progress(i)->NumTapes >= 30) {
			DAT_800d1c60 = 1;
		}
	}

	if (!DAT_800d1c64 && DAT_800d1c60)
		DAT_800d1c40 = 10;
	else if ((DAT_800d1c40 == 10 && !DAT_800d1c60) || DAT_800d1c40 >= GNumSkaters)
		DAT_800d1c40 = 0;

	DAT_800d1c64 = DAT_800d1c60;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	FrontLoader_LoadSet(1);
	Mem_Delete(FrontLoader_TakeFile("select2p.bmp"));
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("selectp.bmp"));
	Pack_DefragmentFully();
	Mess_LoadFont("stats1.fnt", -1, -1, -1);
	Front_InitSkaterPack(1);

	for (i = 0; i < 5; i++) {
		if (i < 4)
			sprintf(name, "bar%d.bmp", i + 1);
		else
			sprintf(name, "bar9.bmp", i + 1);

		bar[i] = new Sprite(name, 1, 32, 128);
		bar[i]->removeFromMemory();
	}

	for (int r = 0; r < 4; r++) {
		rows[r].x = 203;
		rows[r].y = r * 24 + 195;
		rows[r].bar = r;
	}

	rows[0].value = Stats[DAT_800d1c40].Stats[0];
	rows[1].value = Stats[DAT_800d1c40].Stats[1];
	rows[2].value = Stats[DAT_800d1c40].Stats[2];
	rows[3].value = Stats[DAT_800d1c40].Stats[3];

	EraseSprite *bioRect1 = new EraseSprite(203, 287, 188, 33);
	EraseSprite *bioRect2 = new EraseSprite(203, 320, 188, 94);
	Sprite *statBox = new Sprite(203, 180, 200, 95, 1, 32, 128);

	statBox->removeFromMemory();

	char *comps[3] = { "Skate Park", "Burnside", "Roswell" };

	for (i = 0; i < 3; i++) {
		sprintf(name, "medal%d.bmp", i + 1);

		medal[i] = new Sprite(name, 1, 32, 128);
		medal[i]->removeFromMemory();

		medalType[i] = -1;
	}

	Sprite *arrow[4];

	for (i = 0; i < 4; i++) {
		sprintf(name, "levbut%d.bmp", i + 1);

		arrow[i] = new Sprite(name, 1, 32, 128);
		arrow[i]->removeFromMemory();
	}

	uint leftTimer = 0;
	uint rightTimer = 0;
	Sprite *nameBar = new Sprite(0, 54, 478, 33, 1, 32, 128);

	nameBar->removeFromMemory();

	Sprite *topBox = new Sprite(400, 92, 183, 111, 1, 32, 128);
	Sprite *bottomBox = new Sprite(400, 203, 183, 111, 1, 32, 128);

	topBox->removeFromMemory();
	bottomBox->removeFromMemory();
	DrawSync(0);
	Front_InitSkaterSpool1P();
	DrawSync(0);

	SVECTOR rot;

	rot.vx = rot.vy = rot.vz = 0;
	short *viewport = (short *)HiRes_SetupCamera();
	CBruce *pBruce = 0;
	int state = 0;
	int speed = DAT_800d1c58;
	int settle = 0;
	int dir;
	int wait;
	int loadX = 491;
	int noTapes = 1;

	viewport[0] = 640;
	viewport[1] = 480;

	for (int k = 0; k < 11; k++) {
		if (Career_Progress(k)->NumTapes) {
			noTapes = 0;
			break;
		}
	}

	int blink = 96;
	int nameX;
	int nameY;
	int nameL;
	int nameR;
	int nameStep;
	int nameState = 0;
	Sprite *itemBox = 0;
	Sprite *itemBoxBack = 0;
	SLabel6 sStart = "start";
	int hintX[2]; /* unused */
	char first[16];
	int hintY[2];

	hintY[0] = 193;
	hintY[1] = 213;

	if (GGame == 1) {
		itemBox = new Sprite("itembox1.bmp", 1, 32, 128);
		itemBoxBack = new Sprite(395, 313, 240, 102, 0, 32, 128);
	}

	Mess_LoadFont("tapes1.fnt", -1, -1, -1);
	Mess_SetFontScale(255);

	char tapes[3];

	tapes[0] = '0';
	tapes[1] = '0';
	tapes[2] = 0;
	int tapesY = (Mess_TextHeight(tapes) >> 1) + 385 - Mess_TextHeightBelowBaseline(tapes);

	PadBuffer input(-1);

	int textTimer = 0;
	int statTimer = 0;
	int medalTimer = 0;
	uchar *flags = (uchar *)Mem_NewX(10, 1, 1);

	FrontLoader_SetFiles(9);
	Front_SelectSkaterPack(0);

	for (;;) {
		Front_UpdateMusicLoop();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			goto done;

		PollHost();
		HiRes_ClearOTag();
		ResetOT();
		input.update();

		if (!DAT_800d2340)
			Front_SpoolNextSkater1P();
		else
			FrontLoader_Continue();

		int done = 0;

		switch (state) {
			case 0:
			default:
				rot.vy += 10;

				{
					int press = input.getPress();

					if (press == 2) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 1;
						nameState = 4;
						rightTimer = 20;
						dir = 0;
						blink = 96;
					} else if (press == 1) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 2;
						nameState = 4;
						leftTimer = 20;
						dir = 1;
						blink = 96;
					}
				}

				if (!DAT_800d2344 && DAT_800d37e4[DAT_800d1c40]) {
					DAT_800d2344 = 1;

					if (*Stats[DAT_800d1c40].FrontName) {
						Spool_BackupPSX(DAT_800d37b4[DAT_800d1c40]);

						if (Cheat_BigHead) {
							GHeadPart = Stats[DAT_800d1c40].Head;
							GHeadUp = Stats[DAT_800d1c40].Neck;
						}

						ProcessNewPSX(DAT_800d37b4[DAT_800d1c40]);

						GHeadPart = -1;
					}

					pBruce = Front_CreateSkaterModel1P();
					pBruce->mPos.vx = DAT_800d232c;
				}

				break;
			case 1:
				if (DAT_800d2344) {
					pBruce->mPos.vx -= speed << 12;

					if (!settle && pBruce->mPos.vx < DAT_800d2334)
						settle = 2;
					else if (settle > 0 && --settle == 0)
						state = 5;
				} else {
					loadX -= speed;

					if (loadX < 400)
						state = 5;
				}

				break;
			case 2:
				if (DAT_800d2344) {
					pBruce->mPos.vx += speed << 12;

					if (!settle && pBruce->mPos.vx >= DAT_800d2330)
						settle = 2;
					else if (settle > 0 && --settle == 0)
						state = 5;
				} else {
					loadX += speed;

					if (loadX >= 583)
						state = 5;
				}

				break;
			case 3:
				if (DAT_800d2344) {
					pBruce->mPos.vx += speed << 12;

					if (pBruce->mPos.vx >= DAT_800d232c)
						done = 1;
				} else {
					loadX += speed;

					if (loadX >= 491)
						done = 1;
				}

				if (done) {
					state = 0;
					pBruce->mPos.vx = DAT_800d232c;
					int press = input.getPress();

					if (press == 2) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 1;
						nameState = 4;
						rightTimer = 20;
						dir = 0;
					} else if (press == 1) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 2;
						nameState = 4;
						leftTimer = 20;
						dir = 1;
					}
				}

				break;
			case 4:
				if (DAT_800d2344) {
					pBruce->mPos.vx -= speed << 12;

					if (pBruce->mPos.vx < DAT_800d232c)
						done = 1;
				} else {
					loadX -= speed;

					if (loadX < 491)
						done = 1;
				}

				if (done) {
					state = 0;
					pBruce->mPos.vx = DAT_800d232c;
					int press = input.getPress();

					if (press == 2) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 1;
						nameState = 4;
						rightTimer = 20;
						dir = 0;
					} else if (press == 1) {
						SFX_PlayX(SND_SELECTBOARD1, 0x2000, 0);

						state = 2;
						nameState = 4;
						leftTimer = 20;
						dir = 1;
					}
				}

				break;
			case 5:
				state = 6;
				wait = 2;

				if (DAT_800d2344) {
					delete pBruce;

					if (*Stats[DAT_800d1c40].FrontName) {
						ClearRegion(DAT_800d37b4[DAT_800d1c40], 1, 1);
						Spool_RestorePSX(DAT_800d37b4[DAT_800d1c40]);
					}
				}

				if (dir == 0)
					DAT_800d1c40 = Front_StepSkater(DAT_800d1c40, 1, DAT_800d1c60);
				else if (dir == 1)
					DAT_800d1c40 = Front_StepSkater(DAT_800d1c40, -1, DAT_800d1c60);

				DAT_800d2344 = 0;
				Front_ResetSkaterPack();

				if (itemBoxBack)
					itemBoxBack->pack();

				break;
			case 6:
				if (--wait == 0) {
					state = 7;

					if (itemBoxBack)
						itemBoxBack->unpack();

					Front_ResetSkaterPack();
				}

				break;
			case 7:
				DAT_800d2344 = DAT_800d37e4[DAT_800d1c40];

				if (DAT_800d2344) {
					if (*Stats[DAT_800d1c40].FrontName) {
						Spool_BackupPSX(DAT_800d37b4[DAT_800d1c40]);

						if (Cheat_BigHead) {
							GHeadUp = Stats[DAT_800d1c40].Neck;
							GHeadPart = Stats[DAT_800d1c40].Head;
						}

						ProcessNewPSX(DAT_800d37b4[DAT_800d1c40]);

						GHeadPart = -1;
					}

					pBruce = Front_CreateSkaterModel1P();

					if (dir == 1)
						pBruce->mPos.vx = DAT_800d2334;
					else
						pBruce->mPos.vx = DAT_800d2330;
				}

				nameState = 0;
				rows[0].value = Stats[DAT_800d1c40].Stats[0];
				rows[1].value = Stats[DAT_800d1c40].Stats[1];
				rows[2].value = Stats[DAT_800d1c40].Stats[2];
				rows[3].value = Stats[DAT_800d1c40].Stats[3];

				if (dir == 1) {
					state = 3;
					loadX = 400;
				} else {
					state = 4;
					loadX = 583;
				}

				break;
		}

		topBox->draw(400, 92, 0, -1, -1, -1);
		bottomBox->draw(400, 203, 0, -1, -1, -1);

		if (DAT_800d2344) {
			RotMatrixYXZ(&rot, &pBruce->mRotMatrix);

			pBruce->mRotMatrix.m[0][1] = -pBruce->mRotMatrix.m[0][1];
			pBruce->mRotMatrix.m[1][1] = -pBruce->mRotMatrix.m[1][1];
			pBruce->mRotMatrix.m[2][1] = -pBruce->mRotMatrix.m[2][1];

			if (pBruce->mPos.vx >= DAT_800d2334 && pBruce->mPos.vx <= DAT_800d2330) {
				M3d_BuildTransform(pBruce);
				pBruce->UpdateFrame();
				M3d_RenderSetup((SCamera *)MikeCamera, (SViewport *)viewport,
				                pDoubleBuffer->OT3D);
				M3d_Render(MechList);
				M3d_RenderCleanup();
			}

			if (--blink == 0)
				blink = 96;

			if (!noTapes && !state && !Career_Progress(DAT_800d1c40)->NumTapes &&
			    blink >= 23 && GGame == 1) {
				Mess_SetCurrentFont("stats1.fnt");
				Mess_SetRGB(250, 190, 60, 0);
				Mess_SetTextJustify(0);
				int n;

				for (n = 0; n < 15; n++) {
					if (Stats[DAT_800d1c40].Name[n] == ' ')
						break;

					first[n] = Stats[DAT_800d1c40].Name[n];
				}

				first[n] = 0;
				Mess_DrawText(491, hintY[0], sStart, 1, 0, 256, 0);
				Mess_DrawText(491, hintY[1], first, 1, 0, 256, 0);
			}
		} else if (!state) {
			Mess_SetCurrentFont("stats1.fnt");
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(0);
			Mess_DrawText(loadX, 203, "LOADING", 0, 0, 256, 0);
		}

		if (leftTimer) {
			if (--leftTimer < 4)
				arrow[0]->draw(357, 95, 0, -1, -1, -1);
			else
				arrow[2]->draw(357, 95, 0, -1, -1, -1);
		}

		if (rightTimer) {
			if (--rightTimer < 4)
				arrow[1]->draw(585, 95, 0, -1, -1, -1);
			else
				arrow[3]->draw(585, 95, 0, -1, -1, -1);
		}

		if (medalTimer && GGame == 1 && Career_Progress(DAT_800d1c40)->NumTapes) {
			itemBox->draw(395, 313, 0, -1, -1, -1);
			Mess_SetCurrentFont("tapes1.fnt");
			Mess_SetTextJustify(1);
			Mess_DrawText(476, tapesY, tapes, 0, 0, 256, 0);
			int my = 333;

			for (int m = 0; m < 3; m++) {
				if (medalType[m] != -1)
					medal[medalType[m]]->draw(583, my, 0, -1, -1, -1);

				my += 24;
			}
		} else if (GGame == 1 && state == 6) {
			itemBoxBack->draw(395, 313, 0, -1, -1, -1);
		}

		FlickerSync();

		if (statTimer) {
			statTimer--;

			if (statTimer == 2 || statTimer == 3)
				statBox->draw(203, 180, 0, -1, -1, -1);

			int best = 0;

			while (best < 7) {
				if (Career_Progress(DAT_800d1c40)->NumTapes <
				    Stats[DAT_800d1c40].Boards[best + 1].Req)
					break;

				best++;
			}

			bonus[0] = Stats[DAT_800d1c40].Boards[best].Stats[0];
			bonus[1] = Stats[DAT_800d1c40].Boards[best].Stats[1];
			bonus[2] = Stats[DAT_800d1c40].Boards[best].Stats[2];
			bonus[3] = Stats[DAT_800d1c40].Boards[best].Stats[3];

			for (int r = 0; r < 4; r++) {
				int b = rows[r].bar;
				int x = rows[r].x;
				int y = rows[r].y - (bar[b]->m_h >> 1);

				for (i = 0; i < rows[r].value; i++) {
					bar[b]->draw(x, y, 0, -1, -1, -1);

					x += 15;
				}

				for (i = 0; i < bonus[r]; i++) {
					bar[4]->draw(x, y, 0, -1, -1, -1);

					x += 15;
				}
			}

			textTimer = 4;
		} else if (textTimer) {
			textTimer--;

			if (textTimer == 3) {
				bioRect1->restore(203, 287);
				bioRect2->restore(203, 320);
			}

			Mess_SetCurrentFont("stats1.fnt");
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(1);
			Mess_DrawText(203,
			              299 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[0]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[0]),
			              Stats[DAT_800d1c40].pBio[0], 0, 0, 256, 0);
			Mess_DrawText(203,
			              320 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[1]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[1]),
			              Stats[DAT_800d1c40].pBio[1], 0, 0, 256, 0);
			Mess_DrawText(203,
			              341 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[2]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[2]),
			              Stats[DAT_800d1c40].pBio[2], 0, 0, 256, 0);
			Mess_DrawText(203,
			              362 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[3]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[3]),
			              Stats[DAT_800d1c40].pBio[3], 0, 0, 256, 0);
			Mess_DrawText(203,
			              383 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[4]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[4]),
			              Stats[DAT_800d1c40].pBio[4], 0, 0, 256, 0);
			Mess_DrawText(203,
			              404 - (Mess_TextHeight(Stats[DAT_800d1c40].pBio[5]) >> 1) +
			                  Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].pBio[5]),
			              Stats[DAT_800d1c40].pBio[5], 0, 0, 256, 0);

			if (textTimer == 0)
				medalTimer = 3;
		} else if (medalTimer) {
			Mess_SetCurrentFont("tapes1.fnt");
			SSkaterCareer *pCareer = Career_Progress(DAT_800d1c40);

			*flags = pCareer->NumTapes;
			uint n = *flags;

			tapes[0] = n / 10 + '0';
			tapes[1] = n % 10 + '0';
			tapes[2] = 0;
			medalTimer--;
			tapesY = (Mess_TextHeight(tapes) >> 1) + 385 - Mess_TextHeightBelowBaseline(tapes);

			for (int c = 0; c < 3; c++) {
				int j = 0;

				while (strcmp(Levels[j].pLevelName, comps[c]))
					j++;

				*flags = pCareer->Tapes[j];
				int f = *flags;

				if (f & 0x20)
					medalType[c] = 0;
				else if (f & 0x40)
					medalType[c] = 1;
				else if (f & 0x80)
					medalType[c] = 2;
				else
					medalType[c] = -1;
			}
		}

		switch (nameState) {
			case 0:
				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(255);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(0);

				nameR = 320;
				nameX = -Mess_TextWidth(Stats[DAT_800d1c40].Name) >> 1;
				nameStep = (nameR - nameX) / 6;
				nameState = 3;
				nameY = 70 - (Mess_TextHeight(Stats[DAT_800d1c40].Name) >> 1) +
				        Mess_TextHeightAboveBaseline(Stats[DAT_800d1c40].Name);
				nameR -= nameStep;
				nameL = nameX - nameStep;
				break;
			case 3:
				nameX += nameStep;

				if ((nameX < nameL && nameX < nameR) || (nameL < nameX && nameR < nameX)) {
					statTimer = 5;
					nameState = 5;
				}

				nameBar->draw(0, 54, 0, -1, -1, -1);
				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(255);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(nameX, nameY, Stats[DAT_800d1c40].Name, 0, 0, 256, 0);
				break;
			case 4:
				nameX -= nameStep;

				if ((nameX < nameL && nameX < nameR) || (nameL < nameX && nameR < nameX))
					nameState = 5;

				nameBar->draw(0, 54, 0, -1, -1, -1);
				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(255);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(nameX, nameY, Stats[DAT_800d1c40].Name, 0, 0, 256, 0);
				break;
			case 5:
				nameBar->draw(0, 54, 0, -1, -1, -1);
				Mess_SetCurrentFont("mainf.fnt");
				Mess_SetFontScale(255);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify(0);
				Mess_DrawText(nameX, nameY, Stats[DAT_800d1c40].Name, 0, 0, 256, 0);

				nameState = 2;
				break;
			case 1:
			case 2:
				break;
		}

		SetClippingBox(400, 92, 183, 222);
		Front_ResetDrawArea1P();
		HiRes_DrawOTagNoSync();

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			result = DAT_800d1c40;
			SFX_PlayX(SND_SELECT1, 0x2000, 0);

			if (GCareer.LastSkater[0] != DAT_800d1c40) {
				GCareer.LastSkater[0] = 0;
				GCareer.LastDeck[0] = 0;
				GCareer.LastWheel[0] = 0;
				GCareer.LastTrucks[0] = 1;
				GCareer.LastLevel = 0;
			}

			GCareer.LastSkater[0] = DAT_800d1c40;
			goto done;
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			result = -1;
			Pad[0].MenuBack.Triggered = 0;
			Pad[1].MenuBack.Triggered = 0;
			SFX_PlayX(SND_GOBACK2, 0x2000, 0);
			goto done;
		}
	}

done:

	HiRes_BlankScreen();

	if (Loading)
		FileIO_Sync();

	Mem_Delete(flags);

	if (DAT_800d2344)
		delete pBruce;

	Front_FreeSkaterSpool1P();
	Front_FreeSkaterPack();

	for (i = 0; i < 5; i++)
		delete bar[i];

	delete itemBox;
	delete itemBoxBack;

	for (i = 0; i < 3; i++)
		delete medal[i];

	for (i = 0; i < 4; i++)
		delete arrow[i];

	delete statBox;
	delete nameBar;
	delete topBox;
	delete bottomBox;
	delete bioRect1;
	delete bioRect2;

	SpoolerBlocksFileIO->unloadPRE("Player1.pre");
	Pack_DefragmentFully();
	Mess_SetCurrentFont("tapes1.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("stats1.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("mainf.fnt");

	return result;
}

/* Front_CalcSkaterPreviewBounds: the CBruce preview's screen-space limits (also in src/unnamed/8005bad0.c). The TU
   ends with the out-of-line copy of CItem's inline virtual destructor (FUN_8005bb3c, _._5CItem),
   whose vtable _vt.5CItem g++ puts in .rdata ahead of this TU's jump tables. */
void Front_CalcSkaterPreviewBounds() {
	DAT_800d2334 = (80 - DAT_800d1c50) * DAT_800d1c48;
	DAT_800d232c = DAT_800d1c48 * 171;
	DAT_800d2330 = (DAT_800d1c50 + 263) * DAT_800d1c48;
	DAT_800d2328 = (DAT_800d1c54 - 37) * DAT_800d1c4c;
}

/* The front end's own item class: retail calls it CItem too (its destructor is _._5CItem), but
   it is a plain CClass with its vptr at offset 0, not the world CItem of game/ob.h. Its vtable is
   the front end's copy, 0x800a18e4 (game/vtables.tsv). */
class CFrontItem : public CClass {
  public:
	virtual ~CFrontItem();
};

/* _._10CFrontItem (0x8005bb3c) */
/* An empty destructor; deleting frees through CClass's operator delete. */
CFrontItem::~CFrontItem() {}

static inline int GetReturnScreen() {
	int s = StartScreen;

	StartScreen = 5;
	return s;
}

/* Front_Main__Fv (0x8005bb70) */
/* The front-end screen dispatcher, called once from FUN_8006b3f8 (after the boot
   init) and never returning. No demo twin (THPS2's FrontEnd2_Main is a different, table-driven
   design). Each pass:
     - (re)start: on a soft reset (START+SELECT, DAT_800d1cb8) unload everything and clear it; set
       DemoName to "demoA.rec" and clear the video-restart data.
     - top: in autotest mode (GAutoTest) cycle level/skater/players and launch straight away.
       Otherwise, coming back from a game (StartScreen = 1..6) jump straight back into the screen
       the player left from; else run the title screen (Front_TitleScreen) and then the main menu
       (Front_MainMenu).
     - main-menu choice: 3/4/17/18/19 = 1-player game modes -> skater select (Front_SkaterSelect1P);
       33/34/35/1234567 = 2-player modes -> 2P skater select (Front_SkaterSelect2P); 1553/1554 = replay
       the 1P/2P video restart; 99, 1569-1574, 25202-25207 = FMV movies; 100/101 = credits.
     - 1P: skater select -> board select (Front_BoardSelect1P); 2P: skater select -> board select
       (Front_BoardSelect2P); both then level select (LevelSelect_Run). -1 from a screen backs up one.
     - launch: record the choice, start the game (kickIt -> LaunchGame), tear down, reload
       the shell, and loop (rolling the credits first if the career was finished).
   Register levers: the duplicated case tails (2P modes, return screens 3/4/5) are cross-jumped in
   retail but their extra refs are what give `players`/`menu` their s1/s4 priority; the demo load
   sits in a one-pass do{}while(0) (macro-like), which gives DemoName's hoisted %hi s7 over DB's fp.
   The whole dispatcher is a goto loop (`restart:` ... `goto restart`), not for(;;): with no loop
   notes the pseudos' refs aren't loop-weighted, so local-alloc gives the ++GAutoLevel == 9
   counter v0 ahead of the short-lived constant 9 (as retail). The Competition_Init test is the `&&`
   form `!GVideoReplay && Levels[choice].isCompetition == 1`, which expands as (base+12)+idx and
   gives retail's base-first `addu base,base,idx`. */
void Front_Main(void) {
	int choice;
	int players;
	int mode;
	int skater;
	int menu;
	int r;

	ClearSoftReset();
restart:
	if (DAT_800d1cb8) {
	reset:
		Mess_UnloadAllFonts();
		FrontLoader_UnloadAll();
		Front_FreeSpritesPRE();
		ClearSoftReset();
	}

	strcpy(DemoName, "demoA.rec");
	DAT_800d1cb4 = 1;
	Front_ClearGVideoRestartData();
	Career_Init();
	Front_DisplayScreen();
top:
	FrontLoader_Reset();

	DAT_800d1dc0 = 1;
	GNumberOfPlayers = 1;
	DemoTimer = 1600;
	PadBuffer_SetNumPlayers(2);

	DAT_800d1ca0 = 0;

	if (GAutoTest) {
		mode = 1;
		skater = GAutoPlayer1;
		choice = GAutoLevel;
		SelectedSkater2 = GAutoPlayer2;

		if (!GAutoGame) {
			GNumberOfPlayers = 1;
			GGame = 1;
			players = 1;
		} else {
			players = 2;
			GNumberOfPlayers = 2;
			GGame = 8;
		}

		if (++GAutoPlayer1 == 11)
			GAutoPlayer1 = 0;

		if (++GAutoLevel == 9) {
			GAutoLevel = 0;
			GAutoGame ^= 1;

			if (++GAutoPlayer2 == 11)
				GAutoPlayer2 = 0;
		}

		goto launch;
	}

	Front_LoadSpritesPRE();

	if (StartScreen) {
		Front_SetMusicLoop(SND_MUSICLOOP1);
		Mess_LoadFont("mainf.fnt", -1, -1, -1);
		Mess_SetRGB(128, 128, 128, 0);
		Mess_SetFontScale(256);
		Mess_SetTextJustify(1);
		Mess_SetFontShadowOffset(4);

		mode = 0;
		skater = (unsigned char)GCareer.LastSkater[0];
		players = 1;

		switch (GGame) {
			case 1:
				menu = 16;
				break;
			case 2:
			case 3:
				mode = 1;
				break;
			case 4:
				break;
			case 5:
			case 6:
				mode = 1;
				menu = 32;
				players = 2;
				break;
			case 7:
				mode = 5;
				menu = 32;
				players = 2;
				break;
			case 8:
				mode = 3;
				menu = 32;
				players = 2;
				break;
		}

		switch (GetReturnScreen()) {
			case 2:
				if (players != 1)
					goto board2;

				PadBuffer_SetNumPlayers(1);
				goto board1;
			case 1:
				if (players == 1)
					PadBuffer_SetNumPlayers(1);

				goto level;
			case 3:
				GVideoRestart.NumSkaters = 0;
				menu = 96;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 4:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 5:
				GVideoRestart.NumSkaters = 0;
				menu = 0;
				FrontLoader_SetFiles(0);
				goto mainmenu;
			case 6:
				if (players == 1)
					PadBuffer_SetNumPlayers(1);

				choice = GLevel;
				Mess_SetCurrentFont("mainf.fnt");
				Mess_UnloadFont();
				Front_FreeSpritesPRE();
				goto launch;
		}
	}

	StartScreen = 5;
title:
	FrontLoader_UnloadAll();
	Front_TitleScreen();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont("Start_1.fnt");
	Mess_UnloadFont();

	if (!DemoTimer)
		goto demo;

	menu = 0;
mainmenu:
	Front_SetMusicLoop(SND_MUSICLOOP1);
	Pack_DefragmentFully();

	choice = Front_MainMenu(menu);

	if (DAT_800d1cb8)
		goto reset;

	if (!DemoTimer)
		goto demo;

	Options_SetPreferences();

	mode = 0;
	players = 1;
	menu = 0;

	switch (choice) {
		case -1:
			goto title;
		case 17:
		case 18:
			if (choice == 18) {
				DAT_800d1d80 = 0;
				Career_Init();
			}

			GGame = players;
			menu = 16;
			goto skater1;
		case 3:
			GGame = choice;
			mode = 1;
			goto skater1;
		case 4:
			GGame = 2;
			mode = 1;
			goto skater1;
		case 34:
			mode = 1;
			GGame = 5;
			menu = 32;
			players = 2;
			goto skater2;
		case 33:
			mode = 3;
			GGame = 8;
			menu = 32;
			players = 2;
			goto skater2;
		case 1234567:
			mode = 1;
			GGame = 6;
			menu = 32;
			players = 2;
			goto skater2;
		case 35:
			mode = 5;
			GGame = 7;
			menu = 32;
			players = 2;
			goto skater2;
		case 1553:
			mode = 2;
			choice = GVideoRestart.Level;
			skater = GVideoRestart.Skater;
			StartScreen = 3;
			GGame = GVideoRestart.GGame;
			goto replay;
		case 1554:
			players = 2;
			mode = 2;
			choice = GVideoRestart.Level;
			skater = GVideoRestart.Skater2;
			StartScreen = 3;
			GGame = GVideoRestart.GGame;
			SelectedSkater2 = GVideoRestart.Skater;
		replay:
			FrontLoader_UnloadAll();
			Front_FreeSpritesPRE();
			goto launch;
		case 19:
			DAT_800d1d80 = players;
			GGame = players;
			menu = 16;
			goto skater1;
		case 100:
		case 101:
			goto credits;
	}

	if ((unsigned)(choice - 99) < 2 || (unsigned)(choice - 1569) < 6 ||
	    (unsigned)(choice - 25202) < 6)
		goto movie;

	goto title;

skater1:
	PadBuffer_SetNumPlayers(1);

	GNumberOfPlayers = 1;
	skater = Front_SkaterSelect1P();

	if (DAT_800d1cb8)
		goto reset;

	if (skater == -1) {
		PadBuffer_SetNumPlayers(2);
		goto mainmenu;
	}

	goto board1;
skater2:
	Pack_DefragmentFully();

	skater = Front_SkaterSelect2P();

	if (DAT_800d1cb8)
		goto reset;

	Mess_SetCurrentFont("stats2.fnt");
	Mess_UnloadFont();
	Mess_SetCurrentFont("mainf.fnt");

	if (skater == -1)
		goto mainmenu;

	goto board2;
board1:
	r = Front_BoardSelect1P(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater1;

	goto level;
board2:
	r = Front_BoardSelect2P(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (r == -1)
		goto skater2;
level:
	choice = LevelSelect_Run(skater);

	if (DAT_800d1cb8)
		goto reset;

	if (choice == -1) {
		if (players == 1)
			goto board1;

		if (players == 2)
			goto board2;
	}

	Mess_SetCurrentFont("mainf.fnt");
	Mess_UnloadFont();
	Front_FreeSpritesPRE();
launch:
	Mess_UnloadAllFonts();
	FrontLoader_UnloadAll();
	LoadingScreen_SetCurrentLevel(choice, mode, skater);

	GNumberOfPlayers = players;
	DAT_800d1dc0 = 0;

	if (!GVideoReplay && Levels[choice].isCompetition == 1)
		Competition_Init();

	Front_StopMusicLoop();
	kickIt(choice, skater);
	Init_Restart(PACK_0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(PACK_1, 16);
	HiRes_Init(0);
	Front_DisplayScreen();
	SFX_Init("shell.vab");
	Front_Init();

	if (DAT_800d1cb8) {
		StartScreen = 0;
		goto restart;
	}

	if (!GRollCredits)
		goto top;

	GRollCredits = 0;
	choice = 101;
	goto credits;

demo:
	do {
		Card_LoadDemoReplay((const char *)&DemoName);
	} while (0);

	mode = 2;
	menu = 16;
	players = GVideoRestart.NumSkaters;
	skater = GVideoRestart.Skater;
	choice = GVideoRestart.Level;
	SelectedSkater2 = GVideoRestart.Skater2;
	GGame = GVideoRestart.GGame;
	Front_FreeSpritesPRE();

	StartScreen = 4;
	goto launch;

movie:
	HiRes_BlankScreen();

	DAT_800d1dc0 = 0;
	Front_StopMusicLoop();

	if ((unsigned)(choice - 99) < 2)
		menu = 96;
	else if ((unsigned)(choice - 1569) < 6)
		menu = 1568;
	else if ((unsigned)(choice - 25202) < 6)
		menu = 25200;

	switch (choice) {
		case 99:
			GameFMV_PlayMovie(-26);
			break;
		case 1569:
			GameFMV_PlayMovie(-25);
			break;
		case 1570:
			GameFMV_PlayMovie(-12);
			break;
		case 1571:
			GameFMV_PlayMovie(-13);
			break;
		case 1572:
			GameFMV_PlayMovie(-14);
			break;
		case 1573:
			GameFMV_PlayMovie(-15);
			break;
		case 1574:
			GameFMV_PlayMovie(-16);
			break;
		case 25202:
			GameFMV_PlayMovie(-17);
			break;
		case 25203:
			GameFMV_PlayMovie(-18);
			break;
		case 25204:
			GameFMV_PlayMovie(-19);
			break;
		case 25205:
			GameFMV_PlayMovie(-20);
			break;
		case 25206:
			GameFMV_PlayMovie(-21);
			break;
		case 25207:
			GameFMV_PlayMovie(-11);
			break;
		case 100:
			break;
	}

	if (DoubleBuffer[0].Polys)
		Mem_Delete(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	HiRes_Init(1);
	FrontLoader_SetFiles(0);
	Front_DisplayScreen();
	goto mainmenu;

credits:
	Front_SetMusicLoop(choice == 101 ? SND_MUSICLOOP1 : -1);
	Credits_Roll(choice != 100, 1);

	if (DoubleBuffer[0].Polys)
		Mem_Delete(DoubleBuffer[0].Polys);

	DoubleBuffer[0].Polys = 0;
	HiRes_Init(1);
	FrontLoader_SetFiles(0);
	Front_DisplayScreen();

	GGame = 3;
	goto top;
}

/* ClearSoftReset__Fv (0x8005c678) */
void ClearSoftReset(void) {
	DAT_800d1cb8 = 0;
	DAT_800d2348 = 110;
}

/* Front_CheckSoftReset__Fv (0x8005c68c) */
/* Hold START+SELECT on either pad to count DAT_800d2348 down from 110 (eating the START trigger);
   at zero stop the looping sound and set DAT_800d1cb8. Called from 9 sites. */
void Front_CheckSoftReset(void) {
	using SizedPad::Pad;

	if (!DAT_800d2348) {
		SFX_Stop(DAT_800d1ca4);

		DAT_800d1ca0 = 0;
		DAT_800d1cb8 = 1;
	} else if (Pad[0].Pause.Pressed && Pad[0].Camera.Pressed) {
		DAT_800d2348--;
		Pad[0].Pause.Triggered = 0;
	} else if (Pad[1].Pause.Pressed && Pad[1].Camera.Pressed) {
		DAT_800d2348--;
		Pad[1].Pause.Triggered = 0;
	} else {
		DAT_800d2348 = 110;
	}
}

/* Front_SetMusicLoop__Fi (0x8005c734) */
void Front_SetMusicLoop(int a) {
	DAT_800d1cc0 = GPreferences.MusicVolume * 16383 / 12;

	if (DAT_800d1cbc != a) {
		DAT_800d1cbc = a;
		DAT_800d1cc4 = 1;
	}
}

/* Front_UpdateMusicLoop__Fv (0x8005c780) */
void Front_UpdateMusicLoop(void) {
	int s = SFXLEVEL;

	SFXLEVEL = DAT_800d1cc0;

	if (DAT_800d1cc4) {
		SFX_ModifyVol(DAT_800d1ca4, DAT_800d1ca0, DAT_800d1ca0);

		if (DAT_800d1ca0 >= 0) {
			DAT_800d1ca0 -= 256;
		} else {
			SFX_Stop(DAT_800d1ca4);

			DAT_800d1cc4 = 0;

			if (DAT_800d1cbc != -1)
				DAT_800d1ca4 = SFX_PlayX((ESfx)DAT_800d1cbc, DAT_800d1ca0, 0);

			DAT_800d1ca0 = 0;
		}
	} else if (DAT_800d1cbc != -1) {
		SFX_ModifyVol(DAT_800d1ca4, DAT_800d1ca0, DAT_800d1ca0);

		if (DAT_800d1ca0 < 16384)
			DAT_800d1ca0 += 256;
		else
			DAT_800d1ca0 = 16383;
	}

	SFXLEVEL = s;
}

/* Front_StopMusicLoop__Fv (0x8005c85c) */
void Front_StopMusicLoop(void) {
	Front_SetMusicLoop(-1);

	while (DAT_800d1ca0 >= 0) {
		Front_UpdateMusicLoop();
		PC_FrameWait(1);
	}
}

/* FrontLoader_Reset__Fv (0x8005c8ac) */
void FrontLoader_Reset(void) {
	DAT_800d1ca8 = 0;
	DAT_800d1cac = 0;
}

/* FrontLoader_CheckSet__Fv (0x8005c8bc) */
void FrontLoader_CheckSet(void) {
	if (DAT_800d1cac == 3)
		((volatile SScreenFiles &)DAT_800be478[3]).Num;
}

/* FrontLoader_SetFiles__Fi (0x8005c8dc) */
void FrontLoader_SetFiles(int a) {
	DAT_800d1cac = a;
	FrontLoader_CheckSet();
	DAT_800bec34.setFiles(DAT_800be478[a].pName, DAT_800be478[a].Num);
}

/* FrontLoader_Continue__Fv (0x8005c934) */
void FrontLoader_Continue(void) {
	DAT_800d1ca8 = 1;
	FUN_80065ca4(&DAT_800bec34);

	if (FUN_80065fd4(&DAT_800bec34))
		DAT_800d1ca8 = 2;
}

/* FrontLoader_LoadSet__Fi (0x8005c97c) */
void FrontLoader_LoadSet(int a) {
	short r[4];

	if (DAT_800d1cac != a)
		FrontLoader_SetFiles(a);

	if (DAT_800d1ca8 != 2) {
		if (DAT_800d1dc0) {
			HiRes_BlankScreen();
		} else {
			r[0] = 0;
			r[1] = 0;
			r[2] = 512;
			r[3] = 240;
			PC_ScreenFill(r[0], r[1], r[2], r[3], 0);
		}
	}

	FUN_80065d14(&DAT_800bec34);

	DAT_800d1ca8 = 2;
}

/* FrontLoader_UnloadAll__Fv (0x8005ca10) */
void FrontLoader_UnloadAll(void) {
	if (DAT_800d1ca8)
		FUN_80065d80(&DAT_800bec34);

	DAT_800d1ca8 = 0;
}

/* FrontLoader_TakeFile__FPc (0x8005ca3c) */
char *FrontLoader_TakeFile(char *name) {
	return FUN_80065f1c(&DAT_800bec34, name);
}

/* FrontLoader_Defragment__Fv (0x8005ca64) */
void FrontLoader_Defragment(void) {
	FUN_80065fec(&DAT_800bec34);
}

/* Front_LoadSpritesPRE__Fv (0x8005ca88) */
void Front_LoadSpritesPRE(void) {
	PREManager *p;

	if (SpoolerBlocksFileIO)
		printf("\nPREMgr already loaded");

	p = (PREManager *)__nw__6CClassUi(SIZEOF_PREMANAGER);

	if (p)
		p = FUN_8006613c(p);

	SpoolerBlocksFileIO = p;
	FUN_80066214(p, "Sprites.pre");
}

/* Front_FreeSpritesPRE__Fv (0x8005cae8) */
void Front_FreeSpritesPRE(void) {
	if (SpoolerBlocksFileIO)
		printf("\nNo PREMgr to delete");

	if (SpoolerBlocksFileIO)
		PSX_VT(SpoolerBlocksFileIO)->fn((char *)SpoolerBlocksFileIO + PSX_VT(SpoolerBlocksFileIO)->off, 3);

	SpoolerBlocksFileIO = 0;
}

/* FUN_8005cb54 (0x8005cb54) */
void FUN_8005cb54(void) {
	FUN_8006553c((char **)&DAT_800bec34, 2);
}

/* FUN_8005cb7c (0x8005cb7c) */
void FUN_8005cb7c(void) {
	FUN_80065520(&DAT_800bec34);
}

/* LoadingScreen_Main__Fv (0x8005cba0) */
void LoadingScreen_Main() {
	DISPENV de;
	DRAWENV dr;
	RECT r;

	BackOut = 0;
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	FUN_80092e70(&dr, 0, 0, 512, 240);
	SetDefDispEnv(&de, 0, 0, 512, 240);

	r.x = 0;
	r.y = 0;
	r.w = 512;
	r.h = 240;
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	PC_FrameWait(1);
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);

	dr.dfe = 0;
	dr.isbg = 0;
	dr.r0 = 255;
	dr.g0 = 0;
	dr.b0 = 0;
	PC_DrawSetEnv(&dr);
	PC_ScreenShow(&de);

	if (DAT_800d1cf8 < 6) {
		switch (DAT_800d1cf8) {
			case 0:
				if (Levels[DAT_800d1cf4].isCompetition != 0) {
					FUN_8005cd14();
					return;
				}

				FUN_8005d00c();
				return;
			case 1:
			case 3:
			case 5:
				FUN_8005cd14();
				return;
			case 2:
				FUN_8005d28c();
				break;
		}
	}
}

/* FUN_8005cd14 (0x8005cd14) */
/* : loading-screen setup for the picture screens (DAT_800d1cf8 = 3, 1, 5: their
   picture and background; a level with a load image: its pictures, then screen 4), and the
   "Press X" sprite. */
extern "C" void FUN_8005cd14(void) {
	char *o, *s0, *o2;
	Sprite *s;

	if (DAT_800d1cf8 == 3) {
		FrontLoader_SetFiles(7);
		FrontLoader_LoadSet(7);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile((char *)"loadingg.bmp"));
		FUN_8005dc58(8);
	} else if (DAT_800d1cf8 == 1) {
		FrontLoader_SetFiles(5);
		FrontLoader_LoadSet(5);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile((char *)"loadingt.bmp"));
		FUN_8005dc58(8);
	} else if (DAT_800d1cf8 == 5) {
		FrontLoader_SetFiles(14);
		FrontLoader_LoadSet(14);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile((char *)"load_hor.bmp"));
		FUN_8005dc58(8);
	}

	if (DAT_800d1cf8 == 3 || DAT_800d1cf8 == 1 || DAT_800d1cf8 == 5) {
		o = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
		s = 0;

		if (o)
			s = FUN_80063a5c(o, 8, FrontLoader_TakeFile((char *)"Bckgrnd1.bmp"), 0, 458, 64, 128);

		DAT_800d1d0c = s;
	} else if (Levels[DAT_800d1cf4].isCompetition != 0) {
		FrontLoader_SetFiles(6);
		FrontLoader_LoadSet(6);
		FrontLoader_Defragment();

		s0 = (char *)Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].pBackImage), 1, 1);

		if (s0) {
			FileIO_Load(s0);
			FileIO_Sync();
		}

		o2 = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
		s = 0;

		if (o2)
			s = FUN_80063a5c(o2, 8, s0, 0, 458, 64, 128);

		DAT_800d1d0c = s;
		s0 = (char *)Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].pLoadImage), 1, 1);

		if (s0) {
			FileIO_Load(s0);
			FileIO_Sync();
		}

		FUN_80061a10(s0);
		FUN_8005dc58(5);

		DAT_800d1cf8 = 4;
	}

	s0 = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (s0)
		s = FUN_80063a5c(s0, 8, FrontLoader_TakeFile((char *)"Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = s;
	Pad[0].MenuStart.Triggered = 0;
	Pad[1].MenuStart.Triggered = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}

/* FUN_8005d00c (0x8005d00c) */
/* Front-end screen setup (next to LoadingScreen_Main): loads the level's two
   pictures, builds the three sprites and draws the got-tape markers. buf is an unused 40-byte
   local kept for the frame size. */
extern "C" void FUN_8005d00c(void) {
	char buf[40];
	char *s1, *o, *o2, *o3;
	Sprite *s;
	int i, y;

	FrontLoader_SetFiles(4);
	FrontLoader_LoadSet(4);
	FrontLoader_Defragment();

	s1 = (char *)Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].pLoadImage), 1, 1);
	FileIO_Load(s1);
	FileIO_Sync();
	FUN_80061a10(s1);
	FUN_8005dc58(6);

	s1 = (char *)Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].pBackImage), 1, 1);
	FileIO_Load(s1);
	FileIO_Sync();

	o = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (o)
		s = FUN_80063a5c(o, 8, s1, 0, 458, 64, 128);

	DAT_800d1d0c = s;
	o2 = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (o2)
		s = FUN_80063a5c(o2, 8, FrontLoader_TakeFile((char *)"Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = s;
	DAT_800d1d18 = *Career_Tapes(DAT_800d234c, DAT_800d1cf4);
	o3 = (char *)__nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (o3)
		s = FUN_80063a5c(o3, 8, FrontLoader_TakeFile((char *)"check2.bmp"), 0, 256, 64, 128);

	DAT_800d1d00 = s;
	HiRes_ClearOTag();
	ResetOT();

	y = 39;

	for (i = 0; i < 5; i++) {
		if ((DAT_800d1d18 >> i) & 1)
			DAT_800d1d00->draw(33, y, 0, -1, -1, -1);

		y += 26;
	}

	HiRes_DrawOTag();

	Pad[0].MenuStart.Triggered = 0;
	Pad[1].MenuStart.Triggered = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}

/* FUN_8005d28c (0x8005d28c) */
void FUN_8005d28c(void) {
	Sprite *p;

	FrontLoader_SetFiles(8);
	FrontLoader_LoadSet(8);
	FrontLoader_Defragment();
	FUN_80061a10(FrontLoader_TakeFile("loadingv.bmp"));
	FUN_8005dc58(6);

	p = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (p) {
		p = FUN_80063a5c(p, 8, FrontLoader_TakeFile("Bckgrnd2.bmp"), 0, 458, 64, 128);
	}

	DAT_800d1d0c = p;
	Pad[0].MenuStart.Triggered = 0;
	Pad[1].MenuStart.Triggered = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}

/* LoadingScreen_SetCurrentLevel__Fiii (0x8005d35c) */
void LoadingScreen_SetCurrentLevel(int a, int b, int c) {
	DAT_800d1cf4 = a;
	DAT_800d1cf8 = b;
	DAT_800d234c = c;
}

/* FUN_8005d370 (0x8005d370) */
void FUN_8005d370(void) {
	switch (DAT_800d1cf8) {
		case 0:
			LoadingScreen_WaitLevel();
			break;
		case 1:
		case 3:
		case 4:
		case 5:
			LoadingScreen_Wait();
			break;
		case 2:
			FUN_8005da40();
			break;
	}
}

/* LoadingScreen_Wait__Fv (0x8005d3dc) */
/* C++ (front end, THPS1-only; no demo twin). Loading-screen frame for the picture screens
   (DAT_800d1cf8 = 1, 3, 4, 5; dispatched by FUN_8005d370). Called once per load callback while
   loading (DAT_800d1cec = 0) and, after FUN_8005dc8c sets DAT_800d1cec, it loops until the player
   continues: draws the background, the level picture (DAT_800d1d0c) and the blinking "Press X"
   sprite (DAT_800d1d04, toggled by the DAT_800d1d08 counter). START/X or GAutoTest continue
   (sound 6, fade 64); triangle backs out (sound 2). Afterwards it deletes the two sprites. */
void LoadingScreen_Wait(void) {
	using SizedPad::Pad;

	do {
		if (DAT_800d1cec)
			PC_FrameWait(1);

		DrawSync(0);
		HiRes_DrawOTagNoSync();
		HiRes_ClearOTag();
		ResetOT();
		PollHost();

		if (BackOut) {
			DrawColoredBox(0, 0, 512, 240, 0, 0, 0);
			return;
		}

		SetClippingBox(0, 0, 512, 240);

		if (DAT_800d1d14) {
			DAT_800d1d14 = 0;

			if (DAT_800d1cf8 == 4)
				DAT_800d1d0c->draw(DAT_800bf0d4[1], DAT_800bf0e0[1], 0, -1, -1, -1);
			else
				DAT_800d1d0c->draw(DAT_800bf0d4[2], DAT_800bf0e0[2], 0, -1, -1, -1);
		}

		if (DAT_800d1cec) {
			if (--DAT_800d1d08 == 0)
				DAT_800d1d08 = 92;

			if (DAT_800d1d08 == 91) {
				if (DAT_800d1cf8 == 4)
					DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
				else
					DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
			} else if (DAT_800d1d08 == 15) {
				if (DAT_800d1cf8 == 4)
					DAT_800d1d0c->draw(DAT_800bf0d4[1], DAT_800bf0e0[1], 0, -1, -1, -1);
				else
					DAT_800d1d0c->draw(DAT_800bf0d4[2], DAT_800bf0e0[2], 0, -1, -1, -1);
			}
		}

		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			DAT_800d1ce8 = 0;
			BackOut = 1;
			break;
		}

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered || GAutoTest) {
			SFX_PlayX(SND_OLLIE, 8192, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			GameFade = 64;
			break;
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
			Pause(20);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			DAT_800d1ce8 = 0;
			BackOut = 1;
		}
	} while (DAT_800d1cec);

	if (DAT_800d1cec) {
		delete DAT_800d1d04;
		delete DAT_800d1d0c;
	}
}

/* LoadingScreen_WaitLevel__Fv (0x8005d73c) */
/* C++ (front end, THPS1-only; no demo twin). Loading-screen frame for the level loading screen
   (DAT_800d1cf8 = 0, set up by FUN_8005d00c; dispatched by FUN_8005d370). The sibling of
   LoadingScreen_Wait (loadscreenwait.cpp): same loop, but there is no background clear, the picture
   always sits at the screen-0 position, the menu input DAT_800bf104 is updated each frame,
   continuing (START/X) also sets DAT_800d1d80, triangle only eats its own triggers, and the
   tape-marker sprite DAT_800d1d00 is deleted too. */
void LoadingScreen_WaitLevel(void) {
	using SizedPad::Pad;

	do {
		if (DAT_800d1cec)
			PC_FrameWait(1);

		DrawSync(0);
		HiRes_DrawOTagNoSync();
		HiRes_ClearOTag();
		ResetOT();
		PollHost();

		if (BackOut) {
			DrawColoredBox(0, 0, 512, 240, 0, 0, 0);
			return;
		}

		if (DAT_800d1d14) {
			DAT_800d1d14 = 0;
			DAT_800d1d0c->draw(DAT_800bf0d4[0], DAT_800bf0e0[0], 0, -1, -1, -1);
		}

		if (DAT_800d1cec) {
			if (--DAT_800d1d08 == 0)
				DAT_800d1d08 = 92;

			if (DAT_800d1d08 == 91)
				DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
			else if (DAT_800d1d08 == 15)
				DAT_800d1d0c->draw(DAT_800bf0d4[0], DAT_800bf0e0[0], 0, -1, -1, -1);
		}

		DAT_800bf104.update();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			DAT_800d1ce8 = 0;
			BackOut = 1;
			break;
		}

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			SFX_PlayX(SND_OLLIE, 8192, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			GameFade = 64;
			DAT_800d1d80 = 1;
			break;
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(SND_FOOTSLIDE, 8192, 0);
			Pause(20);

			Pad[0].MenuBack.Triggered = 0;
			Pad[1].MenuBack.Triggered = 0;
			DAT_800d1ce8 = 0;
			BackOut = 1;
		}
	} while (DAT_800d1cec);

	if (DAT_800d1cec) {
		delete DAT_800d1d04;
		delete DAT_800d1d0c;
		delete DAT_800d1d00;
	}
}

/* FUN_8005da40 (0x8005da40) */
void FUN_8005da40(void) {
	DrawSync(0);
	HiRes_DrawOTagNoSync();
	HiRes_ClearOTag();
	ResetOT();
	PollHost();
	SetClippingBox(0, 0, 512, 240);

	if (DAT_800d1d14) {
		DAT_800d1d14 = 0;
		DAT_800d1d0c->draw(DAT_800bf0dc[0], DAT_800bf0e8[0], 0, -1, -1, -1);
	}

	Pad_Update();

	if (FUN_800718d0())
		Pad[0].MenuAccept.Triggered = 1;

	if (Pad[0].MenuAccept.Triggered && DemoTimer == 0) {
		SFX_PlayX(SND_FOOTSLIDE, 0x2000, 0);
		Pause(20);
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		DAT_800d1ce8 = 0;
		BackOut = 1;
	}

	if (DAT_800d1cec) {
		if (DAT_800d1d0c)
			PSX_VT(DAT_800d1d0c)->fn((char *)DAT_800d1d0c + PSX_VT(DAT_800d1d0c)->off, 3);

		if (DemoTimer == 0) {
			GameFade = 64;
			return;
		}

		Mess_LoadFont("ingame1.fnt", -1, -1, -1);
		HiRes_ClearOTag();
		ResetOT();
		Mess_SetTextJustify(0);
		Mess_SetScale(255);
		Mess_SetRGB(150, 100, 25, 0);
		HiRes_DrawOTag();
		Mess_UnloadFont();
	}
}

/* FUN_8005dc58 (0x8005dc58) */
void FUN_8005dc58(int a) {
	DAT_800d1ce4 = 1;
	DAT_800d1ce8 = 1;
	DAT_800d1cdc = 0;
	DAT_800d1ce0 = a * 60;
	DAT_800d1cd8 = Vblanks;
}

/* FUN_8005dc8c (0x8005dc8c) */
void FUN_8005dc8c(void) {
	DAT_800d1ce4 = 0;
	DAT_800d1ce8 = 0;
	DAT_800d1cec = 1;
	DAT_800d1d14 = 1;
	DAT_800d1d08 = 1;
	FUN_8005d370();
}

/* FUN_8005dcc4 (0x8005dcc4) */
/* Draws the countdown bar started by FUN_8005dc58: elapsed vblanks since the start, capped at the
   total, as a 400x8 bar (dark red, filling) at one of three screen positions; a full blue bar once
   time is up or when no total is set. ClearImage is ClearImage (it returns a value). */
void FUN_8005dcc4(void) {
	RECT r;

	if (BackOut)
		return;

	if (!DAT_800d1ce4)
		return;

	DAT_800d1cdc = Vblanks - DAT_800d1cd8;

	if (DAT_800d1ce0 < DAT_800d1cdc)
		DAT_800d1cdc = DAT_800d1ce0;

	if (DAT_800d1cf8 == 4) {
		r.x = *(ushort *)DAT_800bf0f0;
		r.y = *(ushort *)DAT_800bf0fc;
	} else if (DAT_800d1cf8 == 0) {
		r.x = *(ushort *)DAT_800bf0ec;
		r.y = *(ushort *)DAT_800bf0f8;
	} else {
		r.x = *(ushort *)DAT_800bf0f4;
		r.y = *(ushort *)DAT_800bf100;
	}

	r.w = 0;
	r.h = 8;

	if (DAT_800d1ce0 == 0 || DAT_800d1ce0 < DAT_800d1cdc) {
		r.w = 400;
		PC_ScreenFill(r.x, r.y, r.w, r.h, 0xff0000);
	} else {
		r.w = (DAT_800d1cdc * 400) / DAT_800d1ce0;
		PC_ScreenFill(r.x, r.y, r.w, r.h, 0x000096);
	}
}

/* FUN_8005ddec (0x8005ddec) */
/* The teardown (_GLOBAL_.D) of DAT_800bf104 (FUN_8005de08): the inlined ~CClass puts CClass's
   vtable back into its vptr. */
void FUN_8005ddec(void) {
	volatile int pad[4];

	*(char **)&DAT_800bf104 = DAT_800a1b94;
}

/* FUN_8005de48 (0x8005de48) */
void FUN_8005de48(char *p, int f) {
	*(char **)(p + 0) = DAT_800a1b94;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* Front_MainMenu__Fi (0x8005de7c) */
int Front_MainMenu(int a) {
	int i, j, r;
	Sprite *o, *p, *q;

	Pack_DefragmentFully();
	FrontLoader_LoadSet(0);
	FrontLoader_Defragment();

	if (DAT_800d1d24) {
		for (i = 0; i < GNumSkaters; i++) {
			Career_Progress(i)->NumTapes = 0;
			Career_Progress(i)->NumGold = 0;
			Career_Progress(i)->NumSilver = 0;
			Career_Progress(i)->NumBronze = 0;

			for (j = 0; j < 10; j++) {
				*Career_Tapes(i, j) = 255;
				Career_Progress(i)->NumTapes += 5;
				Career_Progress(i)->NumGold += 1;
				Career_Progress(i)->NumSilver += 1;
				Career_Progress(i)->NumBronze += 1;
			}

			/* PC: the mods' levels too (their tapes aren't in the career's counts) */
			for (; j < MAX_LEVELS - 1; j++)
				*Career_Tapes(i, j) = 255;
		}
	}

	DownloadBMP16x2(FrontLoader_TakeFile("main_h.bmp"));
	Mess_LoadFont("mainf.fnt", FrontLoader_TakeFile("mainf.fnt"), -1, -1, -1);
	Mess_SetFontScale(200);

	o = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (o)
		o = __6SpritePciii(o, "levbut2.bmp", 1, 32, 128);

	DAT_800d2358 = o;
	p = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (p)
		p = __6SpritePciii(p, "levbut1.bmp", 1, 32, 128);

	DAT_800d235c = p;
	FrontLoader_SetFiles(1);

	r = FUN_8005e754(a);
	q = DAT_800d2358;

	if (q)
		PSX_VT(q)->fn((char *)q + PSX_VT(q)->off, 3);

	q = DAT_800d235c;

	if (q)
		PSX_VT(q)->fn((char *)q + PSX_VT(q)->off, 3);

	return r;
}

/* FUN_8005e09c (0x8005e09c) */
int FUN_8005e09c(int *p) {
	int v = *p;
	int m = 15;
	int o = v;

	if (v < 16) {
		*p = 0;
		return v;
	}

	do {
		v >>= 4;
		m <<= 4;
	} while (v & 240);

	*p = o & ~m;
	return v;
}

/* Front_RunMenu__FPPcPiiiT1PFP12VerticalMenu_vT1T1i (0x8005e0e0) */
int Front_RunMenu(char **a0, int *a1, int a2, int a3, int *a4, void (*a5)(VerticalMenu *), int *a6,
                  int *a7, int a8) {
	VerticalMenu *s2;
	PadBuffer *s1;
	int s3, s0, i, r, t;
	SControl *P;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	s3 = FUN_8005e09c(a4);

	if (s3)
		return s3 + a3;

	s2 = (VerticalMenu *)__nw__6CClassUi(SIZEOF_VERTICALMENU);

	if (s2)
		s2 = __12VerticalMenuiiiPPciPiT6i(s2, 100, 398, 15, a0, a2, a6, a7, a8);

	s2->setTriangles(DAT_800d2358, DAT_800d235c);

	for (i = 0; i < a2; i++) {
		if (a1[i])
			FUN_800648c0(s2, i, 1);
	}

	s0 = -1;

	if (a5 && a5 == Front_ControllerOptionsHandler)
		s0 = DAT_800d1d28;

	s1 = (PadBuffer *)__nw__6CClassUi(SIZEOF_PADBUFFER);

	if (s1)
		s1 = __9PadBufferi(s1, s0);

	DemoTimer = 1600;

	while (s3 == 0) {
		Front_UpdateMusicLoop();

		if (strcmp(*a0, "CAREER MODE") == 0) {
			if (DemoTimer)
				DemoTimer--;
			else
				s3 = 128;
		}

		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			s3 = 0x666;
			break;
		}

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

		r = s1->getPress();

		if (r == 3) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			s2->up();
		}

		if (r == 4) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			s2->down();
		}

		if (r)
			DemoTimer = 1600;

		s1->update();

		if (a5)
			a5(s2);

		PSX_VT(s2)->fn2((char *)s2 + PSX_VT(s2)->off2);
		HiRes_DrawOTag();

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered ||
		    Pad[1].MenuAccept.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(SND_SELECT1, 8192, 0);

			t = s2->getSelected() + 1;
			s3 = a3 + t;
		}

		P = Pad;

		if (P[0].MenuBack.Triggered || P[1].MenuBack.Triggered) {
			Pad_ClearTriggers(P);
			Pad_ClearTriggers(P + 1);
			SFX_PlayX(SND_GOBACK2, 8192, 0);

			s3 = -1;
		}
	}

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();
		s2->erase();
		HiRes_DrawOTag();
	} while (i < 2);

	if (s2)
		PSX_VT(s2)->fn((char *)s2 + PSX_VT(s2)->off, 3);

	if (s1)
		PSX_VT(s1)->fn((char *)s1 + PSX_VT(s1)->off, 3);

	return s3;
}

/* FUN_8005e4ec (0x8005e4ec) */
int FUN_8005e4ec(char **a, int b, int c) {
	int r, i;
	VerticalMenu *m;
	PadBuffer *n;

	r = 0;
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	m = (VerticalMenu *)__nw__6CClassUi(SIZEOF_VERTICALMENU);

	if (m)
		m = (VerticalMenu *)FUN_800652e4((char **)m, 100, 398, 15, a, b, 0, 0, 0);

	n = (PadBuffer *)__nw__6CClassUi(SIZEOF_PADBUFFER);

	if (n)
		n = __9PadBufferi(n, -1);

	do {
		FrontLoader_Continue();
		Pad_Update();
		PollHost();
		HiRes_ClearOTag();
		ResetOT();
		PSX_VT(m)->fn2((char *)m + PSX_VT(m)->off2);
		HiRes_DrawOTag();

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered ||
		    Pad[1].MenuAccept.Triggered || Pad[0].MenuBack.Triggered ||
		    Pad[1].MenuBack.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(SND_SELECT1, 0x2000, 0);

			r = -1;
		}

		if (c != -1) {
			c--;

			if (c < 0)
				r = -1;
		}
	} while (r == 0);

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();
		m->erase();
		HiRes_DrawOTag();
	} while (i < 2);

	if (m)
		PSX_VT(m)->fn((char *)m + PSX_VT(m)->off, 3);

	if (n)
		PSX_VT(n)->fn((char *)n + PSX_VT(n)->off, 3);

	return r;
}

/* FUN_8005e728 (0x8005e728) */
void FUN_8005e728(VerticalMenu *a) {
	FUN_800648c0(a, 1, Pad[1].Type == 0);
}

/* FUN_8005e754 (0x8005e754) */
int FUN_8005e754(int a) {
	char *t[7];
	int k[7];
	int r, i, path;

	Pad_Update();

	if (Pad[0].SpinLeft.Pressed && Pad[0].SpinRight.Pressed)
		DAT_800d1d2c = 1;

	for (;;) {
		*(SMenuText6 *)t = DAT_800a1c44;
		t[6] = s_quit;

		for (i = 0; i < 7; i++)
			k[i] = 0;

		path = a != 0;
		r = Front_RunMenu(t, k, 7, 0, &a, FUN_8005e728, 0, 0, 0);

		if (r != -1) {
			switch (r) {
				case 1:
					r = FUN_8005e8e0(a);
					break;
				case 2:
					r = FUN_8005ea00(a);
					break;
				case 6:
					r = FUN_800604c8(a);
					break;
				case 7:
					if (path) {
						r = FUN_80060854(a);
					} else {
						PC_RequestQuit();

						r = -1;
					}

					break;
				case 5:
					r = FUN_80060274(a);
					break;
			}

			if (r == -1) {
				a = 0;
				continue;
			}
		}

		break;
	}

	return r;
}

/* FUN_8005e8e0 (0x8005e8e0) */
int FUN_8005e8e0(int a) {
	SMenuText3 l;
	int k[3];
	char *s;
	int v, r;

	for (;;) {
		l = DAT_800a1c84;
		k[1] = 0;
		k[2] = 0;
		k[0] = DAT_800d1d80 ^ 1;
		v = Front_RunMenu(l.s, k, 3, 16, &a, 0, 0, 0, 0);

		if (v == -1)
			goto ret;

		switch (v) {
			case 18:
				break;
			case 19:
				v = ((int (*)(int))Front_MemCardScreen)(0); /* (an argument it ignores) */

				if (v == 0) {
					s = GameText.pLoadFailed;
					FUN_8005e4ec(&s, 1, -1);

					v = -1;
				}

				break;
			case 17:
				GHasCareerLoaded = 1;
				break;
		}

		if (v == -1) {
			a = 0;
			continue;
		}
	ret:
		r = v;
		break;
	}

	return r;
}

/* FUN_8005ea00 (0x8005ea00) */
/* the menu's result (Front_RunMenu) */
int FUN_8005ea00(int a) {
	SMenuText3 t = DAT_800a1cac;
	SMenuFlags3 z;

	z.v[0] = 0;
	z.v[1] = 0;
	z.v[2] = 0;
	return Front_RunMenu(t.s, z.v, 3, 32, &a, 0, 0, 0, 0);
}

/* Front_GetPadFunctionName__Fi (0x8005ea74) */
char *Front_GetPadFunctionName(int i) {
	using PadFunctionNameView::GameText;
	int cur = DAT_800d1d28;

	switch (i) {
		case 0:
			return ((SPadFunctionNames *)&GameText[55])->GrindSlide;
		case 2:
			return ((SPadFunctionNames *)&GameText[55])->GrabTrick;
		case 3:
			return ((SPadFunctionNames *)&GameText[55])->JumpOllie;
		case 1:
			if (GPreferences.AutoKickEnabled[cur])
				return ((SPadFunctionNames *)&GameText[55])->FlipTrick;

			return ((SPadFunctionNames *)&GameText[55])->FlipTrickKick;
	}

	return 0;
}

/* Front_StorePadMapping__Fi (0x8005eb28) */
void Front_StorePadMapping(int i) {
	using StorePadMappingView::GPreferences;
	using StorePadMappingView::DAT_800bf260;

	GPreferences.ControllerFunction[i].Function0 = DAT_800bf260[i].Function0;
	GPreferences.ControllerFunction[i].Function2 = DAT_800bf260[i].Function2;
	GPreferences.ControllerFunction[i].Function1 = DAT_800bf260[i].Function1;
	GPreferences.ControllerFunction[i].Function3 = DAT_800bf260[i].Function3;
}

/* Front_ControllerOptionsHandler__FP12VerticalMenu (0x8005eb78) */
/* C++ (front end, THPS1-only; no demo twin). The per-frame handler of the CONTROLLER options menu:
   Front_ControllerOptions passes it to the generic menu runner Front_RunMenu, which calls it with the
   VerticalMenu every frame (and, seeing this handler, binds the menu input to player
   DAT_800d1d28's pad). Rows: 0-3 = TRIANGLE/SQUARE/CIRCLE/X (each row shows the function mapped
   to that button), 4 = VIBRATION, 5 = AUTOKICK, 6 = RESET TO DEFAULT, 7 = BACK.
     - Player 2 with no second pad: fake a triangle press on pad 2 so the menu backs out.
     - Up/down/X/start: the row that was just changed (DAT_800d2350) keeps its new function and any
       other button that had the same function takes over the changed button's old one, so the
       mapping stays a permutation; the snapshot DAT_800bf280 is updated, the other rows' labels
       refreshed and the mapping committed to the options (Front_StorePadMapping).
     - Left/right on a button row cycles its function 0..3; on VIBRATION/AUTOKICK sets it
       (left = on) and rebuilds the "%s %s" label (AUTOKICK also changes button labels).
     - X/start on RESET TO DEFAULT restores the identity mapping and turns both options on.
   The four left/right button-row blocks index by the stored selection (`Function[sel]`,
   `DAT_800d2350 = sel`), which keeps retail's unfolded (&map + 4k) + pl*16 address and the s2
   copy of the selection. The row address is built column-first (`b = &map[0].Function[sel];
   f = b + pl * 4;`), which puts the DAT_800bf260 `lui` ahead of `sll pl,4`. `f` is a block-local
   (set once per pseudo), so alias.c knows its base is DAT_800bf260 and lets the reload of *f move
   above the DAT_800d2350 byte store; one function-wide `f` set four times loses its base value. */
void Front_ControllerOptionsHandler(VerticalMenu *menu) {
	using SizedPad::Pad;

	int pl = DAT_800d1d28;
	char buf[32];
	uchar left;
	int sel;

	if (pl == 1 && !Pad[1].Type) {
		Pad[1].MenuBack.Triggered = 1;
		return;
	}

	Pad_ClearTriggers(&Pad[1 - pl]);

	if (Pad[pl].MenuStart.Triggered || Pad[pl].MenuAccept.Triggered || Pad[pl].MenuUp.Triggered ||
	    Pad[pl].MenuDown.Triggered) {
		switch (DAT_800d2350) {
			case 0:
				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[0];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[0];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[0];

				break;
			case 1:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[1];

				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[1];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[1];

				break;
			case 2:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[2];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[2];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[2];

				break;
			case 3:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[3];

				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[3];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[3];

				break;
		}

		DAT_800bf280[pl].Function[0] = DAT_800bf260[pl].Function[0];
		DAT_800bf280[pl].Function[1] = DAT_800bf260[pl].Function[1];
		DAT_800bf280[pl].Function[2] = DAT_800bf260[pl].Function[2];
		DAT_800bf280[pl].Function[3] = DAT_800bf260[pl].Function[3];

		if (menu->getSelected() != 0)
			menu->setString(0, Front_GetPadFunctionName(DAT_800bf260[pl].Function[0]));

		if (menu->getSelected() != 1)
			menu->setString(1, Front_GetPadFunctionName(DAT_800bf260[pl].Function[1]));

		if (menu->getSelected() != 2)
			menu->setString(2, Front_GetPadFunctionName(DAT_800bf260[pl].Function[2]));

		if (menu->getSelected() != 3)
			menu->setString(3, Front_GetPadFunctionName(DAT_800bf260[pl].Function[3]));

		Front_StorePadMapping(pl);
	}

	if (Pad[pl].MenuLeft.Triggered || Pad[pl].MenuRight.Triggered) {
		left = Pad[pl].MenuLeft.Triggered;
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		if ((sel = menu->getSelected()) == 0) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, Front_GetPadFunctionName(*f));
		}

		if ((sel = menu->getSelected()) == 1) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, Front_GetPadFunctionName(*f));
		}

		if ((sel = menu->getSelected()) == 2) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, Front_GetPadFunctionName(*f));
		}

		if ((sel = menu->getSelected()) == 3) {
			SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, Front_GetPadFunctionName(*f));
		}

		if (menu->getSelected() == 4) {
			if (left != GPreferences.VibrationEnabled[pl])
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);

			GPreferences.VibrationEnabled[pl] = left != 0;
			sprintf(buf, "%s %s", GameText.pVibration,
			        GPreferences.VibrationEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);
			menu->setString(4, buf);
		}

		if (menu->getSelected() == 5) {
			if (left != GPreferences.AutoKickEnabled[pl])
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);

			GPreferences.AutoKickEnabled[pl] = left != 0;
			sprintf(buf, "%s %s", GameText.pAutoKick,
			        GPreferences.AutoKickEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);
			menu->setString(5, buf);
			menu->setString(0, Front_GetPadFunctionName(DAT_800bf260[pl].Function[0]));
			menu->setString(1, Front_GetPadFunctionName(DAT_800bf260[pl].Function[1]));
			menu->setString(2, Front_GetPadFunctionName(DAT_800bf260[pl].Function[2]));
			menu->setString(3, Front_GetPadFunctionName(DAT_800bf260[pl].Function[3]));
		}
	}

	if (Pad[pl].MenuStart.Triggered || Pad[pl].MenuAccept.Triggered) {
		if (menu->getSelected() != 7) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		}

		if (menu->getSelected() == 6) {
			DAT_800bf260[pl].Function[0] = 0;
			DAT_800bf260[pl].Function[1] = 1;
			DAT_800bf260[pl].Function[2] = 2;
			DAT_800bf260[pl].Function[3] = 3;
			Front_StorePadMapping(pl);

			GPreferences.VibrationEnabled[pl] = 1;
			sprintf(buf, "%s %s", GameText.pVibration, GameText.pBooleanOn);
			menu->setString(4, buf);

			GPreferences.AutoKickEnabled[pl] = 1;
			sprintf(buf, "%s %s", GameText.pAutoKick, GameText.pBooleanOn);
			menu->setString(5, buf);
			menu->setString(0, Front_GetPadFunctionName(DAT_800bf260[pl].Function[0]));
			menu->setString(1, Front_GetPadFunctionName(DAT_800bf260[pl].Function[1]));
			menu->setString(2, Front_GetPadFunctionName(DAT_800bf260[pl].Function[2]));
			menu->setString(3, Front_GetPadFunctionName(DAT_800bf260[pl].Function[3]));
		}

		if (menu->getSelected() != 4 && menu->getSelected() != 5)
			SFX_PlayX(SND_SELECT1, 8192, 0);
	}
}

static void RestoreArea(void) {
	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
}

static void SetColour(int selected, int dim) {
	if (selected)
		Mess_SetRGB(182, 132, 36, 0);
	else if (dim)
		Mess_SetRGB(72, 72, 72, 0);
	else
		Mess_SetRGB(122, 122, 122, 0);
}

static void Text(int x, int y, int justify, const char *s) {
	Mess_SetTextJustify(justify);
	Mess_DrawText(x, y, (char *)s, 0, 0, 256, 0);
}

/* Up/down with key repeat (a press, then every 4 frames after 18 held) */
static int Repeat(SButton *b) {
	if (b->Triggered) {
		b->Triggered = 0;
		return 1;
	}

	return b->Pressed && b->PressedTime > 18 && (b->PressedTime & 3) == 0;
}

static int Selectable(int row) {
	return s_rows[row].kind != ROW_HEADER;
}

static int AnyMenuButton(SControl *p) {
	return p->MenuUp.Pressed || p->MenuDown.Pressed || p->MenuLeft.Pressed ||
	       p->MenuRight.Pressed || p->MenuAccept.Pressed || p->MenuBack.Pressed;
}

/* Front_ControllerOptions__Fi (0x8005f6bc) */
/* C++ (front end, THPS1-only; no demo twin). The CONTROLLER options screen (opt_ctr.bmp): for the
   player in DAT_800d1d28 it copies the pad mapping out of the options into the edit buffer
   DAT_800bf260, labels the four button rows with their current function (Front_GetPadFunctionName), builds
   the "vibration on/off" and "Autokick on/off" rows, snapshots the mapping into DAT_800bf280 and
   runs the 8-entry menu (TRIANGLE, SQUARE, CIRCLE, X, VIBRATION, AUTOKICK, RESET TO DEFAULT,
   BACK TO OPTIONS; ids 1296..) with Front_ControllerOptionsHandler as the per-frame handler. Backing out restores
   the options background and returns -1. Called from the options menu FUN_80060274. */
int Front_ControllerOptions(int a) {
	using SizedPad::Pad;

	int pl = DAT_800d1d28;
	int gamepad = pl != 0 || PC_InputGamepadConnected(0);
	int sel = 0, top = 0, slot = 0;
	int capturing = 0, settling = 0, dirty = 0, result = -1;
	int base_scale = Mess_GetFontScale();
	int small_scale = base_scale * 3 / 4;

	(void)a;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));
	DrawSync(0);

	s_saved = (ulong *)malloc(s_area.w * s_area.h * 2);

	PC_ScreenRead(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);

	pc_hires_before_draw = RestoreArea;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	DemoTimer = 1600;

	for (;;) {
		SControl *p = &Pad[pl];
		char buf[64];
		int i;

		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			result = 0x666;
			break;
		}

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

		/* ---- input */
		if (capturing) {
			int in = PC_InputCapture(pl, gamepad);

			if (in != PC_BIND_NONE) {
				PC_InputBind(pl, gamepad, (PcAction)s_rows[sel].action, slot, in);

				dirty = 1;
				capturing = 0;
				settling = 1;
				SFX_PlayX(SND_SELECT1, 8192, 0);
			} else if (--capturing == 0) {
				SFX_PlayX(SND_GOBACK2, 8192, 0);
			}

			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		} else if (settling) {
			/* the key just bound may now be a menu button: wait until it is let go */
			settling = AnyMenuButton(p);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		} else {
			int kind = s_rows[sel].kind;
			int left = p->MenuLeft.Triggered, right = p->MenuRight.Triggered;

			if (Repeat(&p->MenuUp)) {
				do
					sel = sel ? sel - 1 : NUM_ROWS - 1;

				while (!Selectable(sel));

				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			}

			if (Repeat(&p->MenuDown)) {
				do
					sel = sel == NUM_ROWS - 1 ? 0 : sel + 1;

				while (!Selectable(sel));

				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			}

			if (left || right) {
				if (kind == ROW_DEVICE && pl == 0) {
					gamepad = right;
					SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
				} else if (kind == ROW_AUTOKICK) {
					GPreferences.AutoKickEnabled[pl] = left;
					SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
				} else if (kind == ROW_VIBRATION) {
					GPreferences.VibrationEnabled[pl] = left;
					SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
				} else if (kind == ROW_ACTION) {
					slot = right;
					SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
				}
			}

			if (p->MenuAccept.Triggered) {
				switch (kind) {
					case ROW_DEVICE:
						if (pl == 0)
							gamepad ^= 1;

						SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
						break;
					case ROW_AUTOKICK:
						GPreferences.AutoKickEnabled[pl] ^= 1;
						SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
						break;
					case ROW_VIBRATION:
						GPreferences.VibrationEnabled[pl] ^= 1;
						SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
						break;
					case ROW_ACTION:
						if (gamepad && !PC_InputGamepadConnected(pl)) {
							SFX_PlayX(SND_GOBACK2, 8192, 0);
						} else {
							capturing = CAPTURE_FRAMES;
							SFX_PlayX(SND_SELECT1, 8192, 0);
						}

						break;
					case ROW_RESET:
						PC_InputResetDevice(pl, gamepad);

						dirty = 1;
						SFX_PlayX(SND_SELECT1, 8192, 0);
						break;
					case ROW_BACK:
						SFX_PlayX(SND_GOBACK2, 8192, 0);
						break;
				}

				if (kind == ROW_BACK)
					break;
			}

			if (kind == ROW_ACTION && PC_InputClearPressed(pl) &&
			    PC_InputGetBind(pl, gamepad, (PcAction)s_rows[sel].action, slot) != PC_BIND_NONE) {
				PC_InputBind(pl, gamepad, (PcAction)s_rows[sel].action, slot, PC_BIND_NONE);

				dirty = 1;
				SFX_PlayX(SND_GOBACK2, 8192, 0);
			}

			if (p->MenuBack.Triggered) {
				SFX_PlayX(SND_GOBACK2, 8192, 0);
				break;
			}

			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		}

		/* keep the selection on screen (and a set's header above its first row) */
		if (sel < top)
			top = sel > 0 && !Selectable(sel - 1) ? sel - 1 : sel;

		if (sel >= top + VISIBLE_ROWS)
			top = sel - VISIBLE_ROWS + 1;

		if (sel <= 1)
			top = 0;

		/* ---- draw */
		Mess_SetFontScale(small_scale);

		for (i = 0; i < VISIBLE_ROWS && top + i < NUM_ROWS; i++) {
			const ControlsRow *r = &s_rows[top + i];
			int y = ROW_TOP + i * ROW_HEIGHT + 7;
			int on = top + i == sel;

			switch (r->kind) {
				case ROW_DEVICE:
					SetColour(on && !gamepad, gamepad || pl != 0);
					Text(SLOT_X0 - 60, y, 0, "KEYBOARD");
					SetColour(on && gamepad, !gamepad);
					Text(SLOT_X1 - 60, y, 0, "GAMEPAD");
					break;
				case ROW_AUTOKICK:
				case ROW_VIBRATION: {
					int value = r->kind == ROW_AUTOKICK ? GPreferences.AutoKickEnabled[pl]
					                                    : GPreferences.VibrationEnabled[pl];

					sprintf(buf, "%s %s",
					        r->kind == ROW_AUTOKICK ? GameText.pAutoKick : GameText.pVibration,
					        value ? GameText.pBooleanOn : GameText.pBooleanOff);
					SetColour(on, 0);
					Text(NAME_X, y, 1, buf);
					break;
				}
				case ROW_HEADER:
					Mess_SetRGB(70, 100, 140, 0);
					Text(NAME_X - 6, y, 1, r->text);
					break;
				case ROW_ACTION: {
					int s;

					SetColour(on, 0);
					Text(NAME_X, y, 1, PC_InputActionLabel((PcAction)r->action));

					for (s = 0; s < PC_BIND_SLOTS; s++) {
						int in = PC_InputGetBind(pl, gamepad, (PcAction)r->action, s);
						const char *label =
						    in == PC_BIND_NONE ? "NONE" : PC_InputInputLabel(gamepad, in);

						if (on && s == slot && capturing)
							label = (capturing >> 4) & 1 ? "" : "PRESS";

						SetColour(on && s == slot, in == PC_BIND_NONE);
						Text(s ? SLOT_X1 : SLOT_X0, y, 0, label);
					}

					break;
				}
				default:
					SetColour(on, 0);
					Text(NAME_X, y, 1, r->text);
					break;
			}
		}

		Mess_SetRGB(70, 100, 140, 0);

		if (top > 0)
			Text(600, ROW_TOP - 14, 2, "MORE");

		if (top + VISIBLE_ROWS < NUM_ROWS)
			Text(600, ROW_TOP + (VISIBLE_ROWS - 1) * ROW_HEIGHT + 7 + 22, 2, "MORE");

		if (capturing)
			sprintf(buf, "%s %s", gamepad ? "PRESS A BUTTON FOR" : "PRESS A KEY FOR",
			        PC_InputActionLabel((PcAction)s_rows[sel].action));
		else if (s_rows[sel].kind == ROW_ACTION)
			sprintf(buf, "%s %s", "ACCEPT TO CHANGE", "   DELETE OR X TO CLEAR");
		else if (s_rows[sel].kind == ROW_DEVICE && pl == 0)
			sprintf(buf, "%s %s", "LEFT OR RIGHT", "TO SWITCH DEVICE");
		else
			buf[0] = 0;

		Mess_SetRGB(122, 122, 122, 0);
		Text(370, HINT_Y, 0, buf);
		Mess_SetFontScale(base_scale);
		HiRes_DrawOTag();
	}

	if (dirty)
		PC_InputSaveSettings();

	/* leave the options background clean for the options menu */
	pc_hires_before_draw = 0;

	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);
	free(s_saved);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	return result;
}

static int StepDistance(int cur, int d, int n) {
	int i;

	if (d > 0) {
		for (i = 0; i < n; i++)
			if (s_distances[i] > cur)
				return s_distances[i];

		return s_distances[0];
	}

	for (i = n - 1; i >= 0; i--)
		if (s_distances[i] < cur)
			return s_distances[i];

	return s_distances[n - 1];
}

/* the menu font has letters and digits only */
static void DistanceLabel(char *buf, int pct) {
	if (pct == PC_DISTANCE_FULL)
		sprintf(buf, "%s %s", "ENTIRE", "LEVEL");
	else if (pct == 100)
		sprintf(buf, "%s %s", "NORMAL", "");
	else if (pct % 100 == 0)
		sprintf(buf, (char *)"%dX", pct / 100);
	else
		sprintf(buf, (char *)"%d PERCENT", pct);
}

/* the resolution a mode uses */
static int *ResOf(PcVideoSettings *s, int mode) {
	return mode == PC_VIDEO_WINDOWED ? &s->win_w : &s->full_w;
}

/* step a mode's resolution through its list (borderless has only the desktop's) */
static void StepResolution(PcVideoSettings *s, int d) {
	int n = PC_VideoResolutionCount(s->mode);
	int *r = ResOf(s, s->mode);
	int i, at = -1;

	if (s->mode == PC_VIDEO_BORDERLESS || n == 0)
		return;

	for (i = 0; i < n; i++) {
		int w, h;

		PC_VideoResolution(s->mode, i, &w, &h);

		if (w == r[0] && h == r[1])
			at = i;
	}

	at = at < 0 ? (d > 0 ? 0 : n - 1) : (at + d + n) % n;
	PC_VideoResolution(s->mode, at, &r[0], &r[1]);
}

/* the same window: nothing to confirm */
static int SameWindow(const PcVideoSettings *a, const PcVideoSettings *b) {
	return a->mode == b->mode && a->win_w == b->win_w && a->win_h == b->win_h &&
	       a->full_w == b->full_w && a->full_h == b->full_h && a->widescreen == b->widescreen;
}

static int SameSettings(const PcVideoSettings *a, const PcVideoSettings *b) {
	return SameWindow(a, b) && a->draw_distance == b->draw_distance &&
	       a->object_distance == b->object_distance && a->fog == b->fog &&
	       a->shadows == b->shadows && a->antialiasing == b->antialiasing;
}

extern "C" void PC_GraphicsOptions(int a) {
	PcVideoSettings cur, edit, old;
	int sel = 0, keep = 0, i;
	int base_scale = Mess_GetFontScale();
	int small_scale = base_scale * 3 / 4;
	(void)a;

	PC_VideoGetSettings(&cur);

	/* fullscreen without a resolution of its own is at the desktop's */
	if (cur.full_w <= 0 && PC_VideoResolutionCount(PC_VIDEO_BORDERLESS))
		PC_VideoResolution(PC_VIDEO_BORDERLESS, 0, &cur.full_w, &cur.full_h);

	edit = cur;
	old = cur;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));
	DrawSync(0);

	s_saved = (ulong *)malloc(s_area.w * s_area.h * 2);

	PC_ScreenRead(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);

	pc_hires_before_draw = RestoreArea;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	DemoTimer = 1600;

	for (;;) {
		SControl *p = &Pad[0];
		char buf[64];

		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

		/* ---- input */
		if (keep) {
			if (p->MenuAccept.Triggered) {
				keep = 0;
				cur = edit;
				PC_VideoSave();
				SFX_PlayX(SND_SELECT1, 8192, 0);
			} else if (p->MenuBack.Triggered || --keep == 0) {
				keep = 0;
				PC_VideoApply(&old);

				edit = old;
				SFX_PlayX(SND_GOBACK2, 8192, 0);
			}
		} else {
			int left = p->MenuLeft.Triggered, right = p->MenuRight.Triggered;
			int d = right - left;

			if (Repeat(&p->MenuUp)) {
				sel = sel ? sel - 1 : GFX_ROWS - 1;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			}

			if (Repeat(&p->MenuDown)) {
				sel = sel == GFX_ROWS - 1 ? 0 : sel + 1;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			}

			if (p->MenuAccept.Triggered && sel < GFX_APPLY)
				d = 1;

			if (d && sel == GFX_MODE) {
				edit.mode = (edit.mode + d + PC_VIDEO_MODES) % PC_VIDEO_MODES;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_RESOLUTION) {
				if (edit.mode == PC_VIDEO_BORDERLESS) {
					SFX_PlayX(SND_GOBACK2, 8192, 0);
				} else {
					StepResolution(&edit, d);
					SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
				}
			} else if (d && sel == GFX_WIDESCREEN) {
				edit.widescreen ^= 1;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_DRAW_DISTANCE) {
				edit.draw_distance = StepDistance(edit.draw_distance, d, DRAW_STEPS);
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_OBJECT_DISTANCE) {
				edit.object_distance = StepDistance(edit.object_distance, d, OBJECT_STEPS);
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_FOG) {
				edit.fog = (edit.fog + d + PC_FOG_MODES) % PC_FOG_MODES;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_SHADOWS) {
				edit.shadows = (edit.shadows + d + PC_SHADOWS_MODES) % PC_SHADOWS_MODES;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			} else if (d && sel == GFX_ANTIALIASING) {
				/* off, 2x, 4x, 8x */
				i = edit.antialiasing >= 8 ? 3 : edit.antialiasing >= 4 ? 2 : edit.antialiasing >= 2;
				i = (i + d + 4) % 4;
				edit.antialiasing = i ? 1 << i : 0;
				SFX_PlayX(SND_MENUCHANGE2, 8192, 0);
			}

			if (p->MenuAccept.Triggered && sel == GFX_APPLY) {
				if (SameSettings(&edit, &cur)) {
					SFX_PlayX(SND_GOBACK2, 8192, 0);
				} else if (SameWindow(&edit, &cur)) {
					PC_VideoApply(&edit);

					cur = edit;
					PC_VideoSave();
					SFX_PlayX(SND_SELECT1, 8192, 0);
				} else {
					old = cur;
					PC_VideoApply(&edit);

					keep = KEEP_FRAMES;
					SFX_PlayX(SND_SELECT1, 8192, 0);
				}
			}

			if ((p->MenuAccept.Triggered && sel == GFX_BACK) || p->MenuBack.Triggered) {
				SFX_PlayX(SND_GOBACK2, 8192, 0);
				break;
			}
		}

		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		/* ---- draw */
		Mess_SetFontScale(small_scale);

		for (i = 0; i < GFX_ROWS; i++) {
			int y = ROW_TOP + (i + (i >= GFX_APPLY)) * ROW_HEIGHT + 7;
			int on = i == sel && !keep;

			switch (i) {
				case GFX_MODE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "DISPLAY MODE");
					Text(SLOT_X1 - 60, y, 0, s_modeLabels[edit.mode]);
					break;
				case GFX_RESOLUTION: {
					int w, h;

					if (edit.mode == PC_VIDEO_BORDERLESS) {
						w = h = 0;

						if (PC_VideoResolutionCount(PC_VIDEO_BORDERLESS))
							PC_VideoResolution(PC_VIDEO_BORDERLESS, 0, &w, &h);
					} else {
						w = ResOf(&edit, edit.mode)[0];
						h = ResOf(&edit, edit.mode)[1];
					}

					SetColour(on, edit.mode == PC_VIDEO_BORDERLESS);
					Text(NAME_X, y, 1, "RESOLUTION");
					sprintf(buf, (char *)"%d X %d", w, h);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				}
				case GFX_WIDESCREEN:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "WIDESCREEN");
					Text(SLOT_X1 - 60, y, 0, edit.widescreen ? "ON" : "OFF");
					break;
				case GFX_DRAW_DISTANCE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "DRAW DISTANCE");
					DistanceLabel(buf, edit.draw_distance);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				case GFX_OBJECT_DISTANCE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "OBJECT DISTANCE");
					DistanceLabel(buf, edit.object_distance);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				case GFX_FOG:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "FOG");
					Text(SLOT_X1 - 60, y, 0, s_fogLabels[edit.fog]);
					break;
				case GFX_SHADOWS:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "SHADOWS");
					Text(SLOT_X1 - 60, y, 0, s_shadowLabels[edit.shadows]);
					break;
				case GFX_ANTIALIASING:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "ANTIALIASING");

					if (edit.antialiasing)
						sprintf(buf, (char *)"%d X", edit.antialiasing);
					else
						strcpy(buf, "OFF");

					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				case GFX_APPLY:
					SetColour(on, SameSettings(&edit, &cur));
					Text(NAME_X, y, 1, "APPLY");
					break;
				default:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "BACK TO OPTIONS");
					break;
			}
		}

		if (keep) {
			Mess_SetRGB(182, 132, 36, 0);
			Text(NAME_X, ROW_TOP + (GFX_ROWS + 2) * ROW_HEIGHT + 7, 1, "KEEP THESE SETTINGS");
			sprintf(buf, (char *)"ACCEPT TO KEEP   BACK TO UNDO   %d", (keep + 59) / 60);
		} else if (sel == GFX_MODE || (sel == GFX_RESOLUTION && edit.mode != PC_VIDEO_BORDERLESS)) {
			sprintf(buf, "%s %s", "LEFT OR RIGHT TO CHANGE", "");
		} else if (sel == GFX_RESOLUTION) {
			sprintf(buf, "%s %s", "BORDERLESS USES THE", "DESKTOP RESOLUTION");
		} else if (sel == GFX_WIDESCREEN) {
			sprintf(buf, "%s %s", "MORE OF THE WORLD AT THE", "SIDES OF WIDE SCREENS");
		} else if (sel == GFX_DRAW_DISTANCE) {
			sprintf(buf, "%s %s", "HOW FAR AWAY THE LEVEL", "AND ITS FOG ARE DRAWN");
		} else if (sel == GFX_OBJECT_DISTANCE) {
			sprintf(buf, "%s %s", "HOW FAR OBJECTS STAY", "DETAILED AND VISIBLE");
		} else if (sel == GFX_FOG) {
			sprintf(buf, "%s %s", "LIGHT FOGS ONLY THE", "FARTHEST PART");
		} else if (sel == GFX_SHADOWS) {
			sprintf(buf, "%s %s", "DREAMCAST LAYS A SOFT SHADOW", "OVER THE LEVEL");
		} else if (sel == GFX_ANTIALIASING) {
			sprintf(buf, "%s %s", "SMOOTHS THE EDGES OF", "POLYGONS");
		} else if (sel == GFX_APPLY && !SameSettings(&edit, &cur)) {
			sprintf(buf, "%s %s", "ACCEPT TO SWITCH", "");
		} else {
			buf[0] = 0;
		}

		Mess_SetRGB(122, 122, 122, 0);
		Text(370, HINT_Y, 0, buf);
		Mess_SetFontScale(base_scale);
		HiRes_DrawOTag();
	}

	/* a change not confirmed yet is undone */
	if (keep)
		PC_VideoApply(&old);

	pc_hires_before_draw = 0;

	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);
	free(s_saved);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
}

/* Front_SoundOptionsHandler__FP12VerticalMenu (0x8005f9ec) */
void Front_SoundOptionsHandler(VerticalMenu *a) {
	using SizedPad::Pad;

	int v, w, u, i, dir;

	XALEVEL = a->getSelected() == 1 ? GPreferences.MusicVolume * 0x555 : 0;
	SFXLEVEL = GPreferences.SFXVolume * 0x555;

	GameFMV_SyncFMV(0);

	if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered || Pad[0].MenuRight.Triggered ||
	    Pad[1].MenuRight.Triggered) {
		dir = 0;

		if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered)
			dir = 1;

		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		if (a->getSelected() == 0) {
			w = GPreferences.SFXVolume + 1;

			if (dir)
				w = GPreferences.SFXVolume - 1;

			GPreferences.SFXVolume = w;

			if (w < 0)
				GPreferences.SFXVolume = 0;

			v = GPreferences.SFXVolume;

			if (v >= 12) {
				GPreferences.SFXVolume = 12;
				v = 12;
			}

			SFXLEVEL = v * 0x555;
			SFX_PlayX(SND_SELECT1, 8192, 0);
		}

		if (a->getSelected() == 1) {
			u = GPreferences.MusicVolume + 1;

			if (dir)
				u = GPreferences.MusicVolume - 1;

			GPreferences.MusicVolume = u;

			if (u < 0)
				GPreferences.MusicVolume = 0;

			if (GPreferences.MusicVolume >= 12)
				GPreferences.MusicVolume = 12;
		}
	}

	if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered || Pad[0].MenuAccept.Triggered ||
	    Pad[1].MenuAccept.Triggered) {
		if (a->getSelected() == 0) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(SND_SELECT1, 8192, 0);
		}

		if (a->getSelected() == 1) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			GameFMV_StopFMV();
			CVector t(0, 0, 0);

			GameFMV_SetStartTrack(2);
			GameFMV_AddSoundSource(&t);

			GameFMV_StartX = -1;
			GameFMV_StartFMV(0);
		}
	}

	for (i = 0; i < 12; i++) {
		v = 32;

		if (i < GPreferences.SFXVolume)
			v = 128;

		DAT_800d1d5c->draw(i * 24 + 206, 204, 0, v, v, v);

		v = 32;

		if (i < GPreferences.MusicVolume)
			v = 128;

		DAT_800d1d60->draw(i * 24 + 206, 284, 0, v, v, v);
	}
}

static inline void SetSoundSourcePos(VECTOR *p, int x, int y, int z) {
	p->vx = x;
	p->vy = y;
	p->vz = z;
}

/* FUN_8005fd50 (0x8005fd50) */
int FUN_8005fd50(int a) {
	VECTOR z;
	char *t[4];
	VECTOR k, c, d;
	Sprite *o;
	Sprite *o2;
	int v;

	Front_SetMusicLoop(-1);
	FrontLoader_LoadSet(12);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("opt_snd.bmp"));

	o = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (o)
		o = __6SpritePciii(o, "opt_dotr.bmp", 1, 256, 256);

	DAT_800d1d5c = o;
	o2 = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (o2)
		o2 = __6SpritePciii(o2, "opt_dotg.bmp", 1, 256, 256);

	DAT_800d1d60 = o2;
	XALEVEL = 0;
	SetSoundSourcePos(&z, 0, 0, 0);
	GameFMV_SetStartTrack(2);
	GameFMV_AddSoundSource((CVector *)&z);

	GameFMV_StartX = -1;
	GameFMV_StartFMV(0);

	*(SMenuText3 *)t = DAT_800a1d7c;
	k.vx = 0;
	k.vy = 0;
	k.vz = 0;
	c.vx = 350;
	c.vy = 350;
	c.vz = 350;
	d.vx = 250;
	d.vy = 330;
	d.vz = 370;
	v = Front_RunMenu(t, (int *)&k, 3, 1312, &a, Front_SoundOptionsHandler, (int *)&c, (int *)&d, 0);

	if (v == -1 || v == 1315) {
		v = -1;
		GameFMV_StopFMV();
		FrontLoader_LoadSet(11);
		FrontLoader_Defragment();
		DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

		XALEVEL = GPreferences.MusicVolume * 1365;
	}

	if (DAT_800d1d60)
		PSX_VT(DAT_800d1d60)->fn((char *)DAT_800d1d60 + PSX_VT(DAT_800d1d60)->off, 3);

	DAT_800d1d60 = 0;

	if (DAT_800d1d5c)
		PSX_VT(DAT_800d1d5c)->fn((char *)DAT_800d1d5c + PSX_VT(DAT_800d1d5c)->off, 3);

	DAT_800d1d5c = 0;
	Front_SetMusicLoop(SND_MUSICLOOP1);
	return v;
}

/* FUN_8005ffac (0x8005ffac) */
void FUN_8005ffac(VerticalMenu *a) {
	char buf[32];
	int s3, s1, t;
	unsigned char *v;
	SControl *s0;

	s0 = Pad;
	FUN_800648c0(a, 1, s0[1].Type == 0);

	if (s0[0].MenuLeft.Triggered || s0[1].MenuLeft.Triggered || s0[0].MenuRight.Triggered ||
	    s0[1].MenuRight.Triggered) {
		s3 = 0;

		if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered)
			s3 = 1;

		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		if (a->getSelected() == 3) {
			if (s3 != GPreferences.TrickTipsEnabled)
				SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

			s1 = s3 != 0;
			GPreferences.TrickTipsEnabled = s1;
			sprintf(buf, "%s %s", GameText.pTrickTips, s1 ? GameText.pBooleanOn : GameText.pBooleanOff);
			a->setString(3, buf);
		}

		if (a->getSelected() == 4) {
			s1 = s3 != 0;

			if (s3 != GPreferences.ScoreDisplayEnabled)
				SFX_PlayX(SND_MENUCHANGE2, 0x2000, 0);

			GPreferences.ScoreDisplayEnabled = s1;
			sprintf(buf, "%s %s", GameText.pScoreDisplay,
			        s1 ? GameText.pBooleanOn : GameText.pBooleanOff);
			a->setString(4, buf);
		}
	}

	if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
	    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
		t = a->getSelected();

		if (t < 5) {
			if (t >= 3) {
				Pad_ClearTriggers(&Pad[0]);
				Pad_ClearTriggers(&Pad[1]);
			}
		}
	}
}

/* FUN_800601fc (0x800601fc) */
/* GameText (LocalText): pLoadComplete, pLoadFailed */
int FUN_800601fc(int) {
	int r = Front_MemCardScreen();
	char *v;

	if (r != -1) {
		if (r == 19) {
			v = GameText.pLoadComplete;
			FUN_8005e4ec(&v, 1, 60);

			r = 0;
		} else if (r == 0) {
			v = GameText.pLoadFailed;
			FUN_8005e4ec(&v, 1, -1);

			r = -1;
		}
	}

	return r;
}

/* FUN_80060274 (0x80060274) */
int FUN_80060274(int a) {
	char A[32];
	char B[32];
	char *t[8];
	int k[8];
	int v, i;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

	for (;;) {
		for (i = 0; i < 5; i++)
			t[i] = DAT_800a1df8.s[i];

		t[5] = s_graphics;
		t[6] = DAT_800a1df8.s[5];
		t[7] = DAT_800a1df8.s[6];

		for (i = 0; i < 8; i++)
			k[i] = 0;

		sprintf(A, "%s %s", GameText.pTrickTips,
		        GPreferences.TrickTipsEnabled ? GameText.pBooleanOn : GameText.pBooleanOff);

		t[3] = A;
		sprintf(B, "%s %s", GameText.pScoreDisplay,
		        GPreferences.ScoreDisplayEnabled ? GameText.pBooleanOn : GameText.pBooleanOff);

		t[4] = B;
		v = Front_RunMenu(t, k, 8, 80, &a, FUN_8005ffac, 0, 0, 0);

		if (v == -1 || v == 88) {
			FrontLoader_LoadSet(0);
			FrontLoader_Defragment();
			DownloadBMP16x2(FrontLoader_TakeFile("main_h.bmp"));

			return -1;
		}

		switch (v) {
			case 81:
			case 82:
				DAT_800d1d28 = v != 81;
				Front_ControllerOptions(a);
				break;
			case 83:
				FUN_8005fd50(a);
				break;
			case 86:
				PC_GraphicsOptions(a);
				break;
			case 87:
				FUN_800601fc(a);
				break;
			default:
				return v;
		}

		a = 0;
	}
}

/* FUN_800604c8 (0x800604c8) */
int FUN_800604c8(int arg) {
	int r;
	SMenuText5 a;
	SMenuFlags5 b;

	for (;;) {
		a = DAT_800a1e4c;
		b.v[0] = 0;
		b.v[1] = 0;
		b.v[2] = 0;
		b.v[3] = 0;
		b.v[4] = 0;
		r = Front_RunMenu(a.s, b.v, 5, 96, &arg, 0, 0, 0, 0);

		if (r == -1)
			goto out;

		switch (r) {
			case 97:
				r = FUN_800609c8(arg);
				break;
			case 98:
				r = FUN_800605c0(arg);
				break;
		}

		if (r != -1)
			goto out;

		arg = 0;
	}
out:
	return r;
}

/* FUN_800605c0 (0x800605c0) */
int FUN_800605c0(int a) {
	int cnt[11];
	SMenuText7 t;
	int k[7];
	int i, j, v, r;

	for (i = 0; i < 11; i++) {
		cnt[i] = 0;

		for (j = 0; j < 9; j++) {
			cnt[i] += (*Career_Tapes(i, j) >> 5) & 1;
		}
	}

	for (;;) {
		t = DAT_800a1eb4;
		k[0] = 0;
		k[6] = 0;
		k[1] = cnt[0] < 3;
		k[2] = cnt[1] < 3;
		k[3] = cnt[2] < 3;
		k[4] = cnt[3] < 3;
		k[5] = cnt[4] < 3;
		v = Front_RunMenu(t.s, k, 7, 0x620, &a, 0, 0, 0, 0);

		if (v != -1) {
			if (v == 0x627)
				v = FUN_80060738(a, cnt);

			if (v == -1) {
				a = 0;
				continue;
			}
		}

		r = v;
		break;
	}

	return r;
}

/* FUN_80060738 (0x80060738) */
int FUN_80060738(int arg, int *p) {
	int flag = 0;
	int i;
	int r;
	SMenuText7 a;
	SMenuFlags7 b;

	for (i = 0; i < 11; i++) {
		if (*Career_Tapes(i, 8) & 224)
			flag = 1;
	}

	a = DAT_800a1f34;
	b.v[0] = 0;
	b.v[1] = p[5] < 3;
	b.v[2] = p[6] < 3;
	b.v[3] = p[7] < 3;
	b.v[4] = p[8] < 3;
	b.v[5] = p[9] < 3;
	b.v[6] = flag == 0;
	r = Front_RunMenu(a.s, b.v, 7, 25200, &arg, 0, 0, 0, 0);

	if (r == 25201)
		r = -1;

	return r;
}

/* FUN_80060854 (0x80060854) */
int FUN_80060854(int a) {
	char *l[2] = { "GET ALL TAPES", "UNLOCK OR LOCK LEVELS" };
	int k[2];
	int v, i, j;

	k[0] = 0;
	k[1] = 0;
	v = Front_RunMenu(l, k, 2, 112, &a, 0, 0, 0, 0);

	if (v == 113) {
		DAT_800d1d7c = 1;
		DAT_800d1d80 = 1;

		for (i = 0; i < GNumSkaters; i++) {
			Career_Progress(i)->NumTapes = 0;
			Career_Progress(i)->NumGold = 0;
			Career_Progress(i)->NumSilver = 0;
			Career_Progress(i)->NumBronze = 0;

			for (j = 0; j < 10; j++) {
				*Career_Tapes(i, j) = 255;
				Career_Progress(i)->NumTapes += 5;
				Career_Progress(i)->NumGold += 1;
				Career_Progress(i)->NumSilver += 1;
				Career_Progress(i)->NumBronze += 1;
			}

			/* PC: the mods' levels too (their tapes aren't in the career's counts) */
			for (; j < MAX_LEVELS - 1; j++)
				*Career_Tapes(i, j) = 255;
		}
	} else if (v == 114) {
		Cheat_LevelSelect = Cheat_LevelSelect ^ 1;

		if (Cheat_LevelSelect == 0)
			printf("\nLevels locked");
		else
			printf("\nLevels unlocked");
	}

	return -1;
}

/* FUN_800609c8 (0x800609c8) */
int FUN_800609c8(int) {
	int fs, i;
	SControl *p;

	fs = Mess_GetFontScale();
	Pack_DefragmentFully();
	Front_InitMemCardMenus(640, (EMemCardState)15, (EPausedStatus)39);
	Front_SetLoadAction(LOAD_REPLAY_HEADER);

	GVInfo1[0].pBlockStart = (ulong *)Mem_NewX(640, 0, 1);
	GVideoRestart.NumSkaters = 0;
	p = Pad;

	do {
		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		HiRes_ClearOTag();
		ResetOT();

		if (pVertMenu != 0)
			pVertMenu->setTriangles(DAT_800d2358, DAT_800d235c);

		Front_MemCardHiResDisplay();
		HiRes_DrawOTag();
	} while (Front_MemCardUpdate() == 18);

	Pad_ClearTriggers(p);
	Pad_ClearTriggers(p + 1);
	SFX_PlayX(SND_SELECT1, 8192, 0);
	Mess_SetFontScale(fs);

	i = 0;

	do {
		HiRes_ClearOTag();
		ResetOT();
		Front_RestoreBackground();

		if (pVertMenu != 0)
			pVertMenu->erase();

		i++;
		HiRes_DrawOTag();
	} while (i < 2);

	DrawSync(0);
	Front_FreeBackground();

	if (pVertMenu != 0)
		Front_FreeMemCardMenus();

	Mem_Delete(GVInfo1[0].pBlockStart);

	switch (GVideoRestart.NumSkaters) {
		case 1:
			return 0x611;
		case 2:
			return 0x612;
		default:
			return -1;
	}
}

/* Front_MemCardScreen__Fv (0x80060ba0) */
/* C++ (front end, THPS1-only; no demo twin). Runs the memory-card screen modally: sets up the
   card menus and a 640-byte block buffer, then loops the front-end frame (Front_MemCardHiResDisplay
   + Front_MemCardUpdate) until the update stops returning 18 (stay). Afterwards it clears the
   pads, plays the back sound, wipes both buffers, frees the menu and the buffer, reloads the
   options, clamps the volumes and copies the pad mapping. Returns -1 (39: cancelled), 19 (35)
   or 0. Called by FUN_800601fc and FUN_8005e8e0.
   Codegen: the exit code is inside the for(;;) as `if (r != 18) { ...; break; }`, which is why
   retail hoists &Pad and the 35 into s-registers before the loop. */
int Front_MemCardScreen() {
	using SizedPad::Pad;

	int scale;
	int r;
	int ret;
	int i;

	scale = Mess_GetFontScale();
	GCareer.Version = 0xdeadbeef;
	Pack_DefragmentFully();
	Front_InitMemCardMenus(640, (EMemCardState)15, (EPausedStatus)39);
	Front_SetLoadAction(LOAD_GAME);

	GVInfo1[0].pBlockStart = (ulong *)Mem_NewX(640, 0, 1);
	GVideoRestart.NumSkaters = 0;

	for (;;) {
		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		HiRes_ClearOTag();
		ResetOT();

		if (pVertMenu)
			pVertMenu->setTriangles(DAT_800d2358, DAT_800d235c);

		Front_MemCardHiResDisplay();
		HiRes_DrawOTag();

		r = Front_MemCardUpdate();

		if (r != 18) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(SND_SELECT1, 8192, 0);
			Mess_SetFontScale(scale);

			if (r == 39)
				ret = -1;
			else if (r == 35)
				ret = 19;
			else
				ret = 0;

			break;
		}
	}

	for (i = 0; i < 2; i++) {
		HiRes_ClearOTag();
		ResetOT();
		Front_RestoreBackground();

		if (pVertMenu)
			pVertMenu->erase();

		HiRes_DrawOTag();
	}

	DrawSync(0);
	Front_FreeBackground();

	if (pVertMenu)
		Front_FreeMemCardMenus();

	Mem_Delete(GVInfo1[0].pBlockStart);
	Options_GetPreferences();

	if (GPreferences.SFXVolume > 12)
		GPreferences.SFXVolume = 12;

	if (GPreferences.MusicVolume > 12)
		GPreferences.MusicVolume = 12;

	for (int p = 0; p < 2; p++) {
		DAT_800bf260[p].Function[0] = GPreferences.ControllerFunction[p][0];
		DAT_800bf260[p].Function[2] = GPreferences.ControllerFunction[p][2];
		DAT_800bf260[p].Function[1] = GPreferences.ControllerFunction[p][1];
		DAT_800bf260[p].Function[3] = GPreferences.ControllerFunction[p][3];
	}

	return ret;
}

/* FUN_80060e38 (0x80060e38) */
void FUN_80060e38(void) {
	DAT_800bf280[0] = DAT_800bf260[0];
	DAT_800bf280[1] = DAT_800bf260[1];
}

/* Front_TitleScreen__Fv (0x80060e98) */
void Front_TitleScreen(void) {
	char *v;
	int w;
	int s0;
	Sprite *p;

	Pack_DefragmentFully();
	Mess_LoadFont("Start_1.fnt", -1, -1, -1);
	Mess_SetFontScale(256);

	v = (char *)Mem_NewX(FileIO_Open("title_h.bmp"), 1, 1);

	if (v) {
		FileIO_Load(v);
		FileIO_Sync();
	}

	DownloadBMP16x2(v);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	DAT_800d1d8c = 0;
	DAT_800d1d8e = 293;
	DAT_800d1d90 = 640;
	DAT_800d1d92 = 128;
	p = (Sprite *)__nw__6CClassUi(SIZEOF_SPRITE);

	if (p)
		p = __6Spriteiiiiiii(p, DAT_800d1d8c, DAT_800d1d8e, DAT_800d1d90, DAT_800d1d92, 1, 32, 128);

	DAT_800d1d88 = p;
	DrawSync(0);

	{
		struct C30 a = DAT_800a1fd0;
		struct C28 b = DAT_800a1ff0;

		s0 = 32;
		w = DAT_800d1cb4;

		if (w) {
			DAT_800d1cb4 = 0;
			DAT_800d1d80 = FUN_800154f4();
		}

		FrontLoader_SetFiles(0);

		DemoTimer = 1600;

		while (1) {
			Front_UpdateMusicLoop();

			if (!DemoTimer)
				break;

			DemoTimer--;
			s0--;
			HiRes_ClearOTag();
			ResetOT();
			FrontLoader_Continue();
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(0);

			if (s0 < 32)
				DAT_800d1d88->draw(DAT_800d1d8c, DAT_800d1d8e, 0, -1, -1, -1);
			else if (s0 < 128) {
				Mess_SetFontShadowOffset(4);
				Mess_DrawText(320, 357, GameText.pPressStart, 1, 0, 256, 0);
			}

			if (!s0)
				s0 = 128;

			Pad_Update();
			Front_CheckSoftReset();

			if (DAT_800d1cb8)
				break;

			Mess_Update();
			Mess_Display();
			HiRes_DrawOTag();

			if (s0 > 128 || !(Pad[0].MenuStart.Triggered ||
			                  Pad[1].MenuStart.Triggered))
				continue;

			Pad[0].MenuStart.Triggered = 0;
			Pad[1].MenuStart.Triggered = 0;
			SFX_PlayX(SND_SELECT1, 8192, 0);
			break;
		}

		if (DAT_800d1d88)
			PSX_VT(DAT_800d1d88)->fn((char *)DAT_800d1d88 + PSX_VT(DAT_800d1d88)->off, 3);
	}
}

/* FUN_80061270 (0x80061270) */
void FUN_80061270(int a) {
	int i;

	if (a != DAT_800d1d84) {
		DAT_800d1d84 = a;

		for (i = 0; i < 2; i++) {
			HiRes_ClearOTag();
			ResetOT();
			DAT_800d1d88->draw(DAT_800d1d8c, DAT_800d1d8e, 0, -1, -1, -1);
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(0);
			Mess_SetFontShadowOffset(4);

			if (a == 0) {
				Mess_SetFontScale(192);
				Mess_DrawText(320, 357, "checking memory card", 1, 0, 256, 0);
			} else if (a == 1) {
				Mess_SetFontScale(144);
				Mess_DrawText(320, 341, "Loading saved data from MEMORY CARD", 1, 0, 256, 0);
				Mess_DrawText(320, 373, "in MEMORY CARD slot 1", 1, 0, 256, 0);
			}

			HiRes_DrawOTag();
		}

		Mess_SetFontScale(255);
		PC_FrameWait(1);
	}
}
