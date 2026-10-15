/* FUN_8005cd14 (0x8005cd14) */
/* C++ (front end): loading-screen setup for the picture screens (DAT_800d1cf8 = 3, 1, 5: their
   picture and background; a level with a load image: its pictures, then screen 4), and the
   "Press X" sprite. */
#include "game/career.h"
#include "game/classsizes.h"
#include "game/fileio.h"
#include "game/frontend.h"
#include "game/mem.h"
#include "game/shellutils.h"
#include "game/padbuttons.h"

extern SLevel Levels[];
extern char Pad[];

int DAT_800d1cf4, DAT_800d1cf8, DAT_800d1cec, DAT_800d1d14;
Sprite *DAT_800d1d04;
Sprite *DAT_800d1d0c;

extern "C" void FUN_80061a10(char *);
extern "C" void FUN_8005dc58(int);
extern "C" char *__nw__6CClassUi(unsigned);
extern "C" Sprite *FUN_80063a5c(char *, int, char *, int, int, int, int);
void Pack_DefragmentFully();
void HiRes_ClearOTag();

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
		o = __nw__6CClassUi(SIZEOF_SPRITE);
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

		o2 = __nw__6CClassUi(SIZEOF_SPRITE);
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

	s0 = __nw__6CClassUi(SIZEOF_SPRITE);
	s = 0;

	if (s0)
		s = FUN_80063a5c(s0, 8, FrontLoader_TakeFile((char *)"Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = s;
	Pad[PAD_MENU_START + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}
