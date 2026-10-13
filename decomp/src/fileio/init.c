/* FileIO_Init__Fv (0x8004c74c) */
#include "names.h"
#ifdef PC_PORT
#include "pc_frame.h"
#endif

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

void FileIO_Init(void) {
	unsigned buf[6];
	int s0;
	unsigned u;

	Loading = 0;
	FUN_8008f208(0);

	while (!FUN_80095b60()) {
		printf("Retry CdInit\n");

		for (s0 = 0; s0 < 100; s0++)
#ifdef PC_PORT
			PC_FrameWait(1);
#else
			VSync(0);
#endif
	}

	while (!FUN_8009085c(buf, "\\CD.HED;1")) {
		while (!FUN_80095b60()) {
			for (s0 = 0; s0 < 100; s0++)
#ifdef PC_PORT
				PC_FrameWait(1);
#else
				VSync(0);
#endif
		}

		for (s0 = 0; s0 < 100; s0++)
#ifdef PC_PORT
			PC_FrameWait(1);
#else
			VSync(0);
#endif
	}

	u = (buf[1] + 2047) >> 11;
	CdControl(2, buf, 0);
	FUN_800958c0(u, Directory, 128);

	while (FUN_80095a5c(1, 0) > 0)
#ifdef PC_PORT
		PC_FrameWait(1);
#else
		VSync(0);
#endif

	while (!FUN_8009085c(buf, "\\CD.WAD;1")) {
		for (s0 = 0; s0 < 100; s0++)
#ifdef PC_PORT
			PC_FrameWait(1);
#else
			VSync(0);
#endif
	}

	WadStart = FUN_800913c0(buf);
	ReadSyncFailures = 0;
}
