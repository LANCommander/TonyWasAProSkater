/* FileIO_Continue__Fv (0x8004cc18) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} R;

int LoadFromPRE, Loading, LastLoading, CurrentPos, FileSize, FileOpen, ReadSyncFailures, NumSectors;
char *Dest;
char FileStart[8];

extern R DAT_800d1b20[];

int CdControl(int, char *, int);
int FUN_800958c0(int, int, int);
int FUN_80095a5c(int, char *);
void Pad_Update(void);
int ClearImage(R *, int, int, int);
void FUN_8005dcc4(void);

/* Loads finish inside FileIO_Load (pc_fileio.h): there are no CD reads to drive, and no load
 * to show the loading screen's countdown bar for. */
void FileIO_Continue(void) {
	LastLoading = Loading;
	Loading = 0;
}
