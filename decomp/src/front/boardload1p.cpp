/* Front_InitBoardPictures1P__Fv (0x80053dbc) flags=-fstructure-noalias */
/* C++ (front end, 1-player board select). Called once by Front_BoardSelect1P (boardselect1p.cpp) before
   its loop: fills in the picture file names of the three columns (decks the skater hasn't got the
   tapes for show "b1lock01.bmp"; trucks and wheels come from fixed tables), resets the streaming
   state that Front_StreamBoardPictures1P (boardstream1p.cpp) works through, and loads the "locked" picture. */
#include "game/shellutils.h"
#include "game/career.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/psyq.h"

extern SCareer GCareer;
extern SStats Stats[];
extern int DAT_800d3394[3];          /* load offset from the start entry (-1: not started) */
extern int DAT_800d33a4[3];          /* start entry */
extern int DAT_800d33b4[3];          /* column fully loaded */
extern int DAT_800d33c4[3];          /* pictures per column */
extern char DAT_800d33d4[3][15][16]; /* picture file names */
extern SlicedImage *DAT_800d36a4[3][15];

SlicedImage *DAT_800d22dc; /* "locked" picture */
int DAT_800d22e0;
char *DAT_800d22e4;
uchar DAT_800d22e8;
int DAT_800d22ec;
int DAT_800d22f0; /* skater */

int Load8BitBMP2(char *, char **, int *, int *, ushort *);

void Front_InitBoardPictures1P(void) {
	DAT_800d3394[0] = -1;
	DAT_800d33c4[0] = 8;
	DAT_800d33b4[0] = 0;
	DAT_800d3394[1] = 0;
	DAT_800d33c4[1] = 3;
	DAT_800d33b4[1] = 0;
	DAT_800d3394[2] = 0;
	DAT_800d33c4[2] = 15;
	DAT_800d33b4[2] = 0;
	DAT_800d22e8 = 1;
	DAT_800d33a4[0] = GCareer.LastDeck[0];
	DAT_800d33a4[1] = GCareer.LastTrucks[0];
	DAT_800d33a4[2] = GCareer.LastWheel[0];

	for (int i = 0; i < DAT_800d33c4[0]; i++) {
		if (GCareer.CharacterProgress[DAT_800d22f0].NumTapes >= Stats[DAT_800d22f0].Boards[i].Req)
			strcpy(DAT_800d33d4[0][i], Stats[DAT_800d22f0].Boards[i].File);
		else
			strcpy(DAT_800d33d4[0][i], "b1lock01.bmp");

		DAT_800d36a4[0][i] = 0;
	}

	char *trucks[3] = { "T1_loose.bmp", "T1_mdium.bmp", "T1_tight.bmp" };

	for (int j = 0; j < DAT_800d33c4[1]; j++) {
		strcpy(DAT_800d33d4[1][j], trucks[j]);

		DAT_800d36a4[1][j] = 0;
	}

	char *wheels[15] = {
		"W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp", "W1brow01.bmp",
		"W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp", "W1gree03.bmp", "W1oran01.bmp",
		"W1purp01.bmp", "W1red_01.bmp", "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp",
	};

	for (int j = 0; j < DAT_800d33c4[2]; j++) {
		strcpy(DAT_800d33d4[2][j], wheels[j]);

		DAT_800d36a4[2][j] = 0;
	}

	DAT_800d22ec = 0;
	DAT_800d22e4 = 0;
	DAT_800d22e0 = 0;
	DAT_800d22e4 = Mem_NewX(FileIO_Open("b1lock01.bmp"), 1, 1);

	FileIO_Load(DAT_800d22e4);
	FileIO_Sync();

	ushort clut;
	char *data;
	int w, h;

	Load8BitBMP2(DAT_800d22e4, &data, &w, &h, &clut);

	DAT_800d22e4 = 0;
	DAT_800d22dc = new SlicedImage(data, w, h, 128, 128, 8, clut);

	DAT_800d22dc->pack();
}
