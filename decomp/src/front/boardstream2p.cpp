/* FUN_8005115c (0x8005115c) flags=-fstructure-noalias */
/* C++ (front end, 2-player board select). Called once a frame by FUN_80051660 (boardselect2p.cpp)
   until every picture is in: streams the deck/trucks/wheels pictures from CD one at a time,
   alternating players and cycling each player's column. The finished load is turned into a
   SlicedImage; the next file is picked outward from the column's current entry (offset 0, n-1, 1,
   n-2, ...) until the middle is reached. Sets DAT_800d22cc when both players' columns are done. */
#include "game/shellutils.h"

extern int Loading;
extern int DAT_800d2c1c[3];             /* pictures per column (8 decks, 3 trucks, 15 wheels) */
extern char DAT_800d2c2c[2][3][15][16]; /* picture file names */
extern SlicedImage *DAT_800d31cc[2][3][15];
extern int DAT_800d333c[2][3]; /* load offset from the start entry (-1: not started) */
extern int DAT_800d335c[2][3]; /* start entry */
extern int DAT_800d337c[2][3]; /* column fully loaded */

int DAT_800d22a8;      /* entry being loaded */
int DAT_800d22ac;      /* player being loaded */
int DAT_800d22b8[2];   /* column being loaded, per player */
char *DAT_800d22c0[2]; /* load buffer, per player */
uchar DAT_800d22c8;
int DAT_800d22cc; /* all pictures loaded */

extern "C" {
	int FUN_8008eebc(int);
	void FileIO_Continue__Fv(void);
	int FileIO_Open__FPCc(char *);
	char *Mem_NewX__FUiii(int, int, int);
	void FileIO_Load__FPv(char *);
}

int Load8BitBMP2(char *, char **, int *, int *, ushort *);

extern "C" void FUN_8005115c(void) {
	FUN_8008eebc(-1);

	int col = DAT_800d22b8[DAT_800d22ac];

	FileIO_Continue__Fv();

	if (DAT_800d22cc)
		DAT_800d22c8 = 1;

	if (DAT_800d22c8 == 1 && !Loading && !DAT_800d22cc) {
		if (DAT_800d333c[DAT_800d22ac][col] >= 0) {
			ushort clut;
			char *data;
			int w, h;

			Load8BitBMP2(DAT_800d22c0[DAT_800d22ac], &data, &w, &h, &clut);

			DAT_800d22c0[DAT_800d22ac] = 0;
			DAT_800d31cc[DAT_800d22ac][col][DAT_800d22a8] =
			    new SlicedImage(data, w, h, 128, 128, 8, clut);
		}

		if (!DAT_800d337c[DAT_800d22ac][col]) {
			int off = DAT_800d333c[DAT_800d22ac][col];

			if (off == -1) {
				DAT_800d333c[DAT_800d22ac][col] = 0;
			} else {
				int n = DAT_800d2c1c[col];

				if (off < n / 2)
					DAT_800d333c[DAT_800d22ac][col] = n - off - 1;
				else if (off > n / 2)
					DAT_800d333c[DAT_800d22ac][col] = n - off;
				else
					DAT_800d337c[DAT_800d22ac][col] = 1;
			}
		}

		DAT_800d22ac = 1 - DAT_800d22ac;
		int k;

		for (k = 0; k < 3; k++) {
			if (++DAT_800d22b8[DAT_800d22ac] >= 3)
				DAT_800d22b8[DAT_800d22ac] = 0;

			if (!DAT_800d337c[DAT_800d22ac][DAT_800d22b8[DAT_800d22ac]])
				break;
		}

		if (k == 3) {
			if (DAT_800d22ac == 1)
				DAT_800d22cc = 1;
		} else {
			int c = DAT_800d22b8[DAT_800d22ac];

			DAT_800d22a8 = DAT_800d335c[DAT_800d22ac][c] + DAT_800d333c[DAT_800d22ac][c];

			if (DAT_800d22a8 >= DAT_800d2c1c[c])
				DAT_800d22a8 -= DAT_800d2c1c[c];

			int size = FileIO_Open__FPCc(DAT_800d2c2c[DAT_800d22ac][c][DAT_800d22a8]);

			if (size > 0)
				FileIO_Load__FPv(DAT_800d22c0[DAT_800d22ac] = Mem_NewX__FUiii(size, 1, 1));
		}
	}
}
