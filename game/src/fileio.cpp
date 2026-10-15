/* FILEIO.CPP: file loading - opening a file from the loaded PRE archive or the data folder and
   loading it (declarations in game/fileio.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/fileio.h"
#include "game/debug.h"
#include "game/pre.h"
#include "game/psyq.h"
#include "game/pad.h"
#include "pc_fileio.h"
#include "game/padbuttons.h"

#include "game/mem.h"
#include "game/gamefmv.h"
#include "game/globals.h"

extern int SectorsPerFrame;
extern int LastLoading;
extern int WadStart;
extern int ReadSyncFailures;
extern int CurrentPos;
extern int FileOpen;
extern int LoadFromPRE;
extern char *ThePREFile;
extern int ThePREFileSize;
extern const char *Opening;
extern int FileSize;
extern int NumSectors;
extern int Start;
extern char *Dest;

/* FileIO_SetSectorsPerFrame__Fi (0x8004c740) */
void FileIO_SetSectorsPerFrame(int v) {
	SectorsPerFrame = v;
}

/* FileIO_Init__Fv (0x8004c74c) */
/* Files are read from Data/ (pc_fileio.h): no CD to start, no CD.HED directory or CD.WAD. */
void FileIO_Init(void) {
	Loading = 0;
	WadStart = 0;
	ReadSyncFailures = 0;
}

/* FileIO_Open__FPCc (0x8004c8a4) */
/* Fuzzy demo twin (0.67); locals from its MAIN.SYM. Opens a file: from the
   loaded PRE archive if there is one, else by a case-insensitive search of the CD directory
   table. Each entry is {name, pad to 4, sector, size}; an end-of-block marker byte of 255 flashes
   the screen. Sets up the sector count and returns the size rounded to whole sectors.
   153/155 (S29). Two levers moved it from 149:
   - An unsized `extern uchar Pad_50[]` gives retail's split `lui v0` / `lbu a0`.
   - Reusing `len` for the byte count (`len = sectors << 11`). That makes the sll's dest
     set twice, which defeats sched1's set-once boost (birthing_insn_p).
   Residue: retail's sll result is in a1, separate from len (v0, as the demo SYM says),
   and lives across the lbu. Ours shares v0 with len and stores 224c early. A separate
   variable set once gets the boost and loses the order (141-149); one also set in the
   early PRE return gets a1 but costs a copy there (152). The tail statement order was
   searched exhaustively. */
int FileIO_Open(const char *filename) {
	char *header;
	char *scan;
	int len;

	CurrentPos = 0;
	Debug_String(filename, 299, "fileio.cpp");

	LoadFromPRE = 0;

	if (SpoolerBlocksFileIO) {
		ThePREFile = SpoolerBlocksFileIO->getFile((char *)filename, &ThePREFileSize);

		if (ThePREFile) {
			LoadFromPRE = 1;
			FileOpen = 1;
			return ThePREFileSize;
		}
	}

	Debug_String(filename, 323, "fileio.cpp");

	/* the file under Data/ (pc_fileio.h), not the CD directory's entry */
	Opening = filename;
	Start = 0;
	len = PC_FileOpen(filename);

	{
		int sectors = (len + 2047) / 2048;

		NumSectors = sectors;
		len = sectors << 11;
		FileSize = len;
	}

	FileOpen = 1;

	if (Pad[0].TapSpinLeft.Pressed)
		printf("(%d) ");

	Debug_String(filename, 495, "fileio.cpp");
	return FileSize;
}

/* FileIO_Load__FPv (0x8004cb10) */
/* SHeapRange (game/mem.h) */
void FileIO_Load(void *p) {
	int a3, a2;

	if (SpoolerBlocksFileIO && LoadFromPRE) {
		Mem_Copy(p, ThePREFile, ThePREFileSize);

		FileOpen = 0;
		return;
	}

	a3 = DAT_800be09c[0].End - DAT_800be09c[0].Start - Used[0];
	a2 = DAT_800be09c[1].End - DAT_800be09c[1].Start - Used[1];

	if (Pad[0].TapSpinLeft.Pressed)
		printf(" Heaps: Small(0) %d, Large(1) %d", a3, a2);

	GameFMV_StopFMV();

	Dest = (char *)p;
	/* the whole file at once (pc_fileio.h): the load is over when this returns */
	PC_FileRead(p, FileSize);

	Redbook_XACheckSectorOnVSync = 0;
	DAT_800d169c = 1;
	Redbook_XAModeSet = 0;
	CurrentPos = FileSize;
	Loading = 0;
	FileOpen = 0;
}

/* FileIO_Continue__Fv (0x8004cc18) */
/* Loads finish inside FileIO_Load (pc_fileio.h): there are no CD reads to drive, and no load
 * to show the loading screen's countdown bar for. */
void FileIO_Continue(void) {
	LastLoading = Loading;
	Loading = 0;
}

/* FileIO_Sync__Fv (0x8004cdd0) */
void FileIO_Sync(void) {
	/* loads finish inside FileIO_Load (pc_fileio.h) */
	Loading = 0;
	LoadFromPRE = 0;
}
