/* FileIO_Continue__Fv (0x8004cc18) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} R;

int LoadFromPRE, Loading, LastLoading, CurrentPos, FileSize, FileOpen, ReadSyncFailures, NumSectors,
    Dest;
char FileStart[8];

extern R DAT_800d1b20[];

int CdControl(int, char *, int);
int FUN_800958c0(int, int, int);
int FUN_80095a5c(int, char *);
void Pad_Update(void);
int ClearImage(R *, int, int, int);
void FUN_8005dcc4(void);

#ifdef PC_PORT
/* Loads finish inside FileIO_Load (pc_fileio.h): there are no CD reads to drive, and no load
 * to show the loading screen's countdown bar for. */
void FileIO_Continue(void) {
	LastLoading = Loading;
	Loading = 0;
}
#else
void FileIO_Continue(void) {
	char buf[104];
	R r;
	int s0, v;

	if (LoadFromPRE != 0) {
		Loading = 0;
		return;
	}

	LastLoading = Loading;
	s0 = 0;

	switch (Loading) {
		case 0:
			break;
		case 1:
			CurrentPos = 0;

			if (CdControl(2, FileStart, 0) == 0) {
				s0 = 1;
				break;
			}

			Loading = 2;
		case 2:
			if (FUN_800958c0(NumSectors, Dest, 384) == 0) {
				s0 = 1;
				Loading = s0;
				break;
			}

			Loading = 3;
		case 3:
			if (CurrentPos == FileSize) {
				Loading = 0;
				FileOpen = 0;
				break;
			}

			Pad_Update();

			v = FUN_80095a5c(1, buf);

			if (v > 0) {
				ReadSyncFailures = 0;
			} else if (v < 0) {
				ReadSyncFailures = ReadSyncFailures + 1;

				if (ReadSyncFailures >= 5) {
					ReadSyncFailures = 0;
					Loading = 1;
				}

				s0 = 1;
			} else {
				CurrentPos = FileSize;
			}

			break;
	}

	if (s0 != 0) {
		r = DAT_800d1b20[0];
		ClearImage(&r, 255, 0, 0);

		r.y += 256;
		ClearImage(&r, 255, 0, 0);
	}

	FUN_8005dcc4();
}
#endif
