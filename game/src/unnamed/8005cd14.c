/* FUN_8005cd14 (0x8005cd14) */
#include "names.h"
#include "game/padbuttons.h"

typedef struct {
	char p[0xc];
	int fc;
	char q[0x30 - 0x10];
	int f30, f34;
	char r[0x78 - 0x38];
} E;

extern E Levels[];
extern char Pad[];

int DAT_800d1cf4, DAT_800d1cf8, DAT_800d1d04, DAT_800d1d0c, DAT_800d1cec, DAT_800d1d14;

void FrontLoader_SetFiles(int);
void FrontLoader_LoadSet(int);
void FrontLoader_Defragment(void);
int FrontLoader_TakeFile(char *);
void FUN_80061a10(int);
void FUN_8005dc58(int);
int FileIO_Open(int);
int Mem_NewX(int, int, int);
void FileIO_Load(int);
void FileIO_Sync(void);
int __nw__6CClassUi(int);
int FUN_80063a5c(int, int, int, int, int, int, int);
void Pack_DefragmentFully(void);
void HiRes_ClearOTag(void);
void ResetOT(void);

void FUN_8005cd14(void) {
	int o, s0, s1, o2;

	if (DAT_800d1cf8 == 3) {
		FrontLoader_SetFiles(7);
		FrontLoader_LoadSet(7);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile("loadingg.bmp"));
		FUN_8005dc58(8);
	} else if (DAT_800d1cf8 == 1) {
		FrontLoader_SetFiles(5);
		FrontLoader_LoadSet(5);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile("loadingt.bmp"));
		FUN_8005dc58(8);
	} else if (DAT_800d1cf8 == 5) {
		FrontLoader_SetFiles(14);
		FrontLoader_LoadSet(14);
		FrontLoader_Defragment();
		FUN_80061a10(FrontLoader_TakeFile("load_hor.bmp"));
		FUN_8005dc58(8);
	}

	if (DAT_800d1cf8 == 3 || DAT_800d1cf8 == 1 || DAT_800d1cf8 == 5) {
		o = __nw__6CClassUi(24);

		if (o)
			o = FUN_80063a5c(o, 8, FrontLoader_TakeFile("Bckgrnd1.bmp"), 0, 458, 64, 128);

		DAT_800d1d0c = o;
	} else if (Levels[DAT_800d1cf4].fc != 0) {
		FrontLoader_SetFiles(6);
		FrontLoader_LoadSet(6);
		FrontLoader_Defragment();

		s0 = Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].f34), 1, 1);

		if (s0) {
			FileIO_Load(s0);
			FileIO_Sync();
		}

		o2 = __nw__6CClassUi(24);

		if (o2)
			o2 = FUN_80063a5c(o2, 8, s0, 0, 458, 64, 128);

		DAT_800d1d0c = o2;
		s0 = Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].f30), 1, 1);

		if (s0) {
			FileIO_Load(s0);
			FileIO_Sync();
		}

		FUN_80061a10(s0);
		FUN_8005dc58(5);

		DAT_800d1cf8 = 4;
	}

	s0 = __nw__6CClassUi(24);

	if (s0)
		s0 = FUN_80063a5c(s0, 8, FrontLoader_TakeFile("Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = s0;
	Pad[PAD_MENU_START + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}
