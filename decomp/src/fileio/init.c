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

#ifdef PC_PORT
/* Files are read from Data/ (pc_fileio.h): no CD to start, no CD.HED directory or CD.WAD. */
void FileIO_Init(void) {
	Loading = 0;
	WadStart = 0;
	ReadSyncFailures = 0;
}
#else
void FileIO_Init(void) {
	unsigned buf[6];
	int s0;
	unsigned u;

	Loading = 0;
	FUN_8008f208(0);

	while (!FUN_80095b60()) {
		printf("Retry CdInit\n");

		for (s0 = 0; s0 < 100; s0++)
			VSync(0);
	}

	while (!FUN_8009085c(buf, "\\CD.HED;1")) {
		while (!FUN_80095b60()) {
			for (s0 = 0; s0 < 100; s0++)
				VSync(0);
		}

		for (s0 = 0; s0 < 100; s0++)
			VSync(0);
	}

	u = (buf[1] + 2047) >> 11;
	CdControl(2, buf, 0);
	FUN_800958c0(u, Directory, 128);

	while (FUN_80095a5c(1, 0) > 0)
		VSync(0);

	while (!FUN_8009085c(buf, "\\CD.WAD;1")) {
		for (s0 = 0; s0 < 100; s0++)
			VSync(0);
	}

	WadStart = FUN_800913c0(buf);
	ReadSyncFailures = 0;
}
#endif
