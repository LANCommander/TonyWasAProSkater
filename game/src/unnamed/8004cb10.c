/* FileIO_Load__FPv (0x8004cb10) */
#include "names.h"
#include "game/padbuttons.h"
#include "pc_fileio.h"

/* SHeapRange (game/mem.h) */
typedef struct {
	char *a, *b;
} R;

extern R DAT_800be09c[];
extern char Pad[];
extern int Used[2];
extern int Redbook_XACheckSectorOnVSync;
extern int DAT_800d169c;
extern int Redbook_XAModeSet;

extern void *SpoolerBlocksFileIO;
extern int LoadFromPRE;
extern char *ThePREFile;
extern int ThePREFileSize;
extern int FileOpen;
extern void *Dest;
extern int Start;
extern int WadStart;
extern int Loading;
extern int CurrentPos;
extern int FileSize;

void Mem_Copy(void *a, void *b, int c);
void printf(char *a, int b, int c);
void GameFMV_StopFMV(void);
int FUN_800912bc(int a, char *b);
void FileIO_Continue(void);

void FileIO_Load(char *p) {
	int a3, a2;

	if (SpoolerBlocksFileIO && LoadFromPRE) {
		Mem_Copy(p, ThePREFile, ThePREFileSize);

		FileOpen = 0;
		return;
	}

	a3 = DAT_800be09c[0].b - DAT_800be09c[0].a - Used[0];
	a2 = DAT_800be09c[1].b - DAT_800be09c[1].a - Used[1];

	if (Pad[PAD_TAP_SPIN_LEFT])
		printf(" Heaps: Small(0) %d, Large(1) %d", a3, a2);

	GameFMV_StopFMV();

	Dest = p;
	/* the whole file at once (pc_fileio.h): the load is over when this returns */
	PC_FileRead(p, FileSize);

	Redbook_XACheckSectorOnVSync = 0;
	DAT_800d169c = 1;
	Redbook_XAModeSet = 0;
	CurrentPos = FileSize;
	Loading = 0;
	FileOpen = 0;
}
