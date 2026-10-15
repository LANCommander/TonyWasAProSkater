/* FileIO_Open__FPCc (0x8004c8a4) */
/* C++ (FILEIO.CPP). Fuzzy demo twin (0.67); locals from its MAIN.SYM. Opens a file: from the
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
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/debug.h"
#include "game/pre.h"
#include "game/psyq.h"
#include "game/pad.h"
#include "pc_fileio.h"

extern int CurrentPos;
extern int FileOpen;
extern PREManager *SpoolerBlocksFileIO;
extern int LoadFromPRE;
extern char *ThePREFile;
extern int ThePREFileSize;
extern const char *Opening;
extern int FileSize;
extern int NumSectors;
extern int Start;

extern RECT DAT_800d1b20[];
extern SControl Pad[2];
extern uchar Directory[];

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
