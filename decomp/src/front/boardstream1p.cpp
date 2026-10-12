/* FUN_80054134 (0x80054134) flags=-fstructure-noalias */
/* C++ (front end, 1-player board select). The 1-player twin of FUN_8005115c (boardstream2p.cpp),
   called once a frame by FUN_80054510 (boardselect1p.cpp) until every picture is in: streams the
   deck/trucks/wheels pictures from CD one at a time, cycling the columns. The finished load is
   turned into a SlicedImage; the next file is picked outward from the column's current entry
   (offset 0, n-1, 1, n-2, ...) until the middle is reached. Sets DAT_800d22ec when all are done. */
#include "game/shellutils.h"

extern int Loading;
extern int DAT_800d3394[3];          /* load offset from the start entry (-1: not started) */
extern int DAT_800d33a4[3];          /* start entry */
extern int DAT_800d33b4[3];          /* column fully loaded */
extern int DAT_800d33c4[3];          /* pictures per column (8 decks, 3 trucks, 15 wheels) */
extern char DAT_800d33d4[3][15][16]; /* picture file names */
extern SlicedImage *DAT_800d36a4[3][15];

int DAT_800d22d8;   /* entry being loaded */
int DAT_800d22e0;   /* column being loaded */
char *DAT_800d22e4; /* load buffer */
uchar DAT_800d22e8;
int DAT_800d22ec; /* all pictures loaded */

extern "C" {
	void FileIO_Continue__Fv(void);
	int FileIO_Open__FPCc(char *);
	char *Mem_NewX__FUiii(int, int, int);
	void FileIO_Load__FPv(char *);
}

int Load8BitBMP2(char *, char **, int *, int *, ushort *);

extern "C" void FUN_80054134(void) {
	int col = DAT_800d22e0;

	FileIO_Continue__Fv();

	if (DAT_800d22ec)
		DAT_800d22e8 = 1;

	if (DAT_800d22e8 == 1 && !Loading && !DAT_800d22ec) {
		if (DAT_800d3394[col] >= 0) {
			ushort clut;
			char *data;
			int w, h;

			Load8BitBMP2(DAT_800d22e4, &data, &w, &h, &clut);

			DAT_800d22e4 = 0;
			DAT_800d36a4[col][DAT_800d22d8] = new SlicedImage(data, w, h, 128, 128, 8, clut);
		}

		if (!DAT_800d33b4[col]) {
			int off = DAT_800d3394[col];

			if (off == -1) {
				DAT_800d3394[col] = 0;
			} else {
				int n = DAT_800d33c4[col];

				if (off < n / 2)
					DAT_800d3394[col] = n - off - 1;
				else if (off > n / 2)
					DAT_800d3394[col] = n - off;
				else
					DAT_800d33b4[col] = 1;
			}
		}

		int k;

		for (k = 0; k < 3; k++) {
			if (++DAT_800d22e0 >= 3)
				DAT_800d22e0 = 0;

			if (!DAT_800d33b4[DAT_800d22e0])
				break;
		}

		if (k == 3) {
			DAT_800d22ec = 1;
		} else {
			int c = DAT_800d22e0;

			DAT_800d22d8 = DAT_800d33a4[c] + DAT_800d3394[c];

			if (DAT_800d22d8 >= DAT_800d33c4[c])
				DAT_800d22d8 -= DAT_800d33c4[c];

			int size = FileIO_Open__FPCc(DAT_800d33d4[c][DAT_800d22d8]);

			if (size > 0)
				FileIO_Load__FPv(DAT_800d22e4 = Mem_NewX__FUiii(size, 1, 1));
		}
	}
}
