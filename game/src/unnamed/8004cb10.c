/* FileIO_Load__FPv (0x8004cb10) */
#include "names.h"
#include "pc_fileio.h"

/* SHeapRange (game/mem.h) */
typedef struct {
	char *a, *b;
} R;

extern R DAT_800be09c[];
extern char Pad_50[];
extern int Used_4;
extern int Used;
extern int Redbook_XACheckSectorOnVSync;
extern int DAT_800d169c;
extern int Redbook_XAModeSet;

void *SpoolerBlocksFileIO;
int LoadFromPRE;
char *ThePREFile;
int ThePREFileSize;
int FileOpen;
void *Dest;
int Start;
int WadStart;
char FileStart[8];
int Loading;
int CurrentPos;
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

	a3 = DAT_800be09c[0].b - DAT_800be09c[0].a - Used;
	a2 = DAT_800be09c[1].b - DAT_800be09c[1].a - Used_4;

	if (Pad_50[0])
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
