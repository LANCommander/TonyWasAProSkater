/* Front_InitBoardPictures2P__Fv (0x80050c5c) flags=-fstructure-noalias */
/* C++ (front end, 2-player board select). The 2-player twin of Front_InitBoardPictures1P (boardload1p.cpp),
   called once by Front_BoardSelect2P (boardselect2p.cpp) before its loop: fills in each player's picture
   file names for the three columns (the small "2" pictures: a deck file's second letter becomes
   '2'; locked decks show "b2lock01.bmp"), resets the streaming state that Front_StreamBoardPictures2P
   (boardstream2p.cpp) works through, and loads the "locked" picture. */
#include "game/shellutils.h"
#include "game/career.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/psyq.h"

extern SCareer GCareer;
extern SStats Stats[];
extern int DAT_800d2c1c[3];             /* pictures per column */
extern char DAT_800d2c2c[2][3][15][16]; /* picture file names */
extern SlicedImage *DAT_800d31cc[2][3][15];
extern int DAT_800d333c[2][3]; /* load offset from the start entry (-1: not started) */
extern int DAT_800d335c[2][3]; /* start entry */
extern int DAT_800d337c[2][3]; /* column fully loaded */

int DAT_800d22ac;          /* player being loaded */
SlicedImage *DAT_800d22b0; /* "locked" picture */
int DAT_800d22b8[2];       /* column being loaded, per player */
char *DAT_800d22c0[2];     /* load buffer, per player */
uchar DAT_800d22c8;
int DAT_800d22cc;    /* all pictures loaded */
int DAT_800d22d0[2]; /* skater, per player */

int Load8BitBMP2(char *, char **, int *, int *, ushort *);

void Front_InitBoardPictures2P(void) {
	DAT_800d2c1c[0] = 8;
	DAT_800d2c1c[1] = 3;
	DAT_800d2c1c[2] = 15;
	DAT_800d333c[0][0] = -1;
	DAT_800d333c[1][0] = 0;

	for (int p = 0; p < 2; p++) {
		DAT_800d337c[p][0] = 0;
		DAT_800d335c[p][0] = GCareer.LastDeck[0];
		DAT_800d333c[p][1] = 0;
		DAT_800d337c[p][1] = 0;
		DAT_800d335c[p][1] = GCareer.LastTrucks[0];
		DAT_800d333c[p][2] = 0;
		DAT_800d337c[p][2] = 0;
		DAT_800d335c[p][2] = GCareer.LastWheel[0];
		DAT_800d22c8 = 1;

		for (int i = 0; i < DAT_800d2c1c[0]; i++) {
			if (GCareer.CharacterProgress[DAT_800d22d0[p]].NumTapes >=
			    Stats[DAT_800d22d0[p]].Boards[i].Req) {
				strcpy(DAT_800d2c2c[p][0][i], Stats[DAT_800d22d0[p]].Boards[i].File);

				DAT_800d2c2c[p][0][i][1] = '2';
			} else {
				strcpy(DAT_800d2c2c[p][0][i], "b2lock01.bmp");
			}

			DAT_800d31cc[p][0][i] = 0;
		}

		char *trucks[3] = { "T2_loose.bmp", "T2_mdium.bmp", "T2_tight.bmp" };

		for (int j = 0; j < DAT_800d2c1c[1]; j++) {
			strcpy(DAT_800d2c2c[p][1][j], trucks[j]);

			DAT_800d31cc[p][1][j] = 0;
		}

		char *wheels[15] = {
			"W2blue01.bmp", "W2blue02.bmp", "W2blue03.bmp", "W2blue04.bmp", "W2brow01.bmp",
			"W2dark01.bmp", "W2gree01.bmp", "W2gree02.bmp", "W2gree03.bmp", "W2oran01.bmp",
			"W2purp01.bmp", "W2red_01.bmp", "W2red_02.bmp", "W2whit01.bmp", "W2yell01.bmp",
		};

		for (int j = 0; j < DAT_800d2c1c[2]; j++) {
			strcpy(DAT_800d2c2c[p][2][j], wheels[j]);

			DAT_800d31cc[p][2][j] = 0;
		}

		DAT_800d22c0[p] = 0;
		DAT_800d22b8[p] = 0;
	}

	DAT_800d22cc = 0;
	DAT_800d22ac = 0;
	DAT_800d22c0[0] = Mem_NewX(FileIO_Open("b2lock01.bmp"), 1, 1);

	FileIO_Load(DAT_800d22c0[0]);
	FileIO_Sync();

	ushort clut;
	char *data;
	int w, h;

	Load8BitBMP2(DAT_800d22c0[0], &data, &w, &h, &clut);

	DAT_800d22c0[0] = 0;
	DAT_800d22b0 = new SlicedImage(data, w, h, 128, 128, 8, clut);

	DAT_800d22b0->pack();
}
