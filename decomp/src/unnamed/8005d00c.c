/* FUN_8005d00c (0x8005d00c) */
#include "names.h"
#include "game/padbuttons.h"

/* Front-end screen setup (next to LoadingScreen_Main): loads the level's two pictures, builds the three
   sprites and draws the got-tape markers. buf is an unused 40-byte local kept for the frame size. */
typedef struct {
	char p[0x30];
	int f30, f34;
	char q[0x78 - 0x38];
} E;

extern E Levels[];

typedef struct {
	char pad0[12];
	unsigned char Tapes[2];
} SC;

extern SC GCareer[];
extern char Pad[];

int DAT_800d1cf4, DAT_800d234c;
int DAT_800d1d0c, DAT_800d1d04, DAT_800d1d00;
unsigned char DAT_800d1d18;
int DAT_800d1cec, DAT_800d1d14;

void FrontLoader_SetFiles(int);
void FrontLoader_LoadSet(int);
void FrontLoader_Defragment(void);
int FileIO_Open(int);
int Mem_NewX(int, int, int);
void FileIO_Load(int);
void FileIO_Sync(void);
void FUN_80061a10(int);
void FUN_8005dc58(int);
int __nw__6CClassUi(int);
int FUN_80063a5c(int, int, int, int, int, int, int);
int FrontLoader_TakeFile(char *);
void HiRes_ClearOTag(void);
void ResetOT(void);
void Sprite_draw(int, int, int, int, int, int, int);
void HiRes_DrawOTag(void);
void Pack_DefragmentFully(void);

void FUN_8005d00c(void) {
	char buf[40];
	int s1, o, o2, o3, i, y;

	FrontLoader_SetFiles(4);
	FrontLoader_LoadSet(4);
	FrontLoader_Defragment();

	s1 = Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].f30), 1, 1);
	FileIO_Load(s1);
	FileIO_Sync();
	FUN_80061a10(s1);
	FUN_8005dc58(6);

	s1 = Mem_NewX(FileIO_Open(Levels[DAT_800d1cf4].f34), 1, 1);
	FileIO_Load(s1);
	FileIO_Sync();

	o = __nw__6CClassUi(24);

	if (o)
		o = FUN_80063a5c(o, 8, s1, 0, 458, 64, 128);

	DAT_800d1d0c = o;
	o2 = __nw__6CClassUi(24);

	if (o2)
		o2 = FUN_80063a5c(o2, 8, FrontLoader_TakeFile("Pressx.bmp"), 0, 440, 64, 128);

	DAT_800d1d04 = o2;
	DAT_800d1d18 = GCareer[DAT_800d234c].Tapes[DAT_800d1cf4];
	o3 = __nw__6CClassUi(24);

	if (o3)
		o3 = FUN_80063a5c(o3, 8, FrontLoader_TakeFile("check2.bmp"), 0, 256, 64, 128);

	DAT_800d1d00 = o3;
	HiRes_ClearOTag();
	ResetOT();

	y = 39;

	for (i = 0; i < 5; i++) {
		if ((DAT_800d1d18 >> i) & 1)
			Sprite_draw(DAT_800d1d00, 33, y, 0, -1, -1, -1);

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
