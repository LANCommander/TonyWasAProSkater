/* FileIO_Load__FPv (0x8004cb10) */
#include "names.h"

typedef struct {
	int a, b;
} R;

extern R DAT_800be09c[];
extern char Pad_50[];
extern int Used_4;
extern int Used;
extern int Redbook_XACheckSectorOnVSync;
extern int DAT_800d169c;
extern int Redbook_XAModeSet;

int SpoolerBlocksFileIO;
int LoadFromPRE;
int ThePREFile;
int ThePREFileSize;
int FileOpen;
int Dest;
int Start;
int WadStart;
char FileStart[8];
int Loading;
int CurrentPos;

void Mem_Copy(void *a, void *b, int c);
void printf(char *a, int b, int c);
void GameFMV_StopFMV(void);
int FUN_800912bc(int a, char *b);
void FileIO_Continue(void);

void FileIO_Load(char *p) {
	int a3, a2;

	if (SpoolerBlocksFileIO && LoadFromPRE) {
		Mem_Copy(p, (void *)ThePREFile, ThePREFileSize);

		FileOpen = 0;
		return;
	}

	a3 = DAT_800be09c[0].b - DAT_800be09c[0].a - Used;
	a2 = DAT_800be09c[1].b - DAT_800be09c[1].a - Used_4;

	if (Pad_50[0])
		printf(" Heaps: Small(0) %d, Large(1) %d", a3, a2);

	GameFMV_StopFMV();

	Dest = (int)p;
	FUN_800912bc(Start / 2048 + WadStart, FileStart);

	Loading = 1;
	Redbook_XACheckSectorOnVSync = 0;
	DAT_800d169c = 1;
	Redbook_XAModeSet = 0;
	CurrentPos = 0;
	FileIO_Continue();
}
