/* FileIO_Init__Fv (0x8004c74c) */
#include "names.h"

extern char s_Retry_CdInit[], s_CD_HED_1[], s_CD_WAD_1[], Directory[];

int Loading, WadStart, ReadSyncFailures;

void FUN_8008f208(int);
void FUN_8008c190(char *);
void FUN_8008eebc(int);
int FUN_80095b60(void);
int FUN_8009085c(unsigned *, char *);
void FUN_8008f264(int, unsigned *, int);
void FUN_800958c0(int, char *, int);
int FUN_80095a5c(int, int);
int FUN_800913c0(unsigned *);

void FileIO_Init(void) {
	unsigned buf[6];
	int s0;
	unsigned u;

	Loading = 0;
	FUN_8008f208(0);

	while (!FUN_80095b60()) {
		FUN_8008c190(s_Retry_CdInit);

		for (s0 = 0; s0 < 100; s0++)
			FUN_8008eebc(0);
	}

	while (!FUN_8009085c(buf, s_CD_HED_1)) {
		while (!FUN_80095b60()) {
			for (s0 = 0; s0 < 100; s0++)
				FUN_8008eebc(0);
		}

		for (s0 = 0; s0 < 100; s0++)
			FUN_8008eebc(0);
	}

	u = (buf[1] + 2047) >> 11;
	FUN_8008f264(2, buf, 0);
	FUN_800958c0(u, Directory, 128);

	while (FUN_80095a5c(1, 0) > 0)
		FUN_8008eebc(0);

	while (!FUN_8009085c(buf, s_CD_WAD_1)) {
		for (s0 = 0; s0 < 100; s0++)
			FUN_8008eebc(0);
	}

	WadStart = FUN_800913c0(buf);
	ReadSyncFailures = 0;
}
