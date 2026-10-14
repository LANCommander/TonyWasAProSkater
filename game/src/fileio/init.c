/* FileIO_Init__Fv (0x8004c74c) */
#include "names.h"

extern char Directory[];

int Loading, WadStart, ReadSyncFailures;

void FUN_8008f208(int);
void printf(char *);
void VSync(int);
int FUN_80095b60(void);
int FUN_8009085c(unsigned *, char *);
void CdControl(int, unsigned *, int);
void FUN_800958c0(int, char *, int);
int FUN_80095a5c(int, int);
int FUN_800913c0(unsigned *);

/* Files are read from Data/ (pc_fileio.h): no CD to start, no CD.HED directory or CD.WAD. */
void FileIO_Init(void) {
	Loading = 0;
	WadStart = 0;
	ReadSyncFailures = 0;
}
