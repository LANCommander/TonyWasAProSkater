/* FUN_8005d00c (0x8005d00c) */
/* C++ (front end). Front-end screen setup (next to LoadingScreen_Main): loads the level's two
   pictures, builds the three sprites and draws the got-tape markers. buf is an unused 40-byte
   local kept for the frame size. */
#include "game/career.h"
#include "game/fileio.h"
#include "game/frontend.h"
#include "game/mem.h"
#include "game/shellutils.h"
#include "game/padbuttons.h"

extern SLevel Levels[];

typedef struct {
	char pad0[12];
	unsigned char Tapes[2];
} SC;

extern SC GCareer[];
extern char Pad[];

int DAT_800d1cf4, DAT_800d234c;
Sprite *DAT_800d1d0c, *DAT_800d1d04, *DAT_800d1d00;
unsigned char DAT_800d1d18;
int DAT_800d1cec, DAT_800d1d14;

extern "C" void FUN_80061a10(char *);
extern "C" void FUN_8005dc58(int);
extern "C" char *__nw__6CClassUi(unsigned);
extern "C" Sprite *FUN_80063a5c(char *, int, char *, int, int, int, int);
void HiRes_ClearOTag();
void HiRes_DrawOTag();
void Pack_DefragmentFully();

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

	o = __nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (o)
		s = FUN_80063a5c(o, 8, s1, 0, 458, 64, 128);

	DAT_800d1d0c = s;
	o2 = __nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (o2)
		s = FUN_80063a5c(o2, 8, FrontLoader_TakeFile((char *)"Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = s;
	DAT_800d1d18 = GCareer[DAT_800d234c].Tapes[DAT_800d1cf4];
	o3 = __nw__6CClassUi(SIZEOF_SPRITE);
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

	Pad[PAD_MENU_START + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}
