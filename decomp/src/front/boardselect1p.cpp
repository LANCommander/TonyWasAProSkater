/* Front_BoardSelect1P__Fi (0x80054510) */
/* C++ (front end, 1-player board select; called from the screen dispatcher Front_Main). Three
   columns (deck, trucks, wheels) each scroll a pair of SlicedImage pictures vertically through a
   clipped box; left/right moves between columns, up/down scrolls the selected column. A text bar
   shows the deck's name (or "Get %d tapes to unlock" / "Unlock in career mode"), and five stat
   rows draw bar sprites: the skater's base stats, the best unlocked board's, and the difference
   to the chosen deck. X/Start accepts an unlocked deck and saves the choice into the career
   record; triangle backs out. Returns 1 (accept) or -1 (back). No demo twin. */
#include "game/shellutils.h"
#include "game/pre.h"
#include "game/career.h"
#include "game/pad.h"
#include "game/mess.h"
#include "game/sfx.h"
#include "game/psyq.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/main.h"

extern SControl Pad[];
extern SCareer GCareer;
extern SStats Stats[];
extern BoardStats GBoardStats[];
extern int DAT_800be438[];
extern int DAT_800d33c4[];
extern SlicedImage *DAT_800d36a4[][15];
extern int DAT_800d1cb8;
extern int GGame;
extern PREManager *SpoolerBlocksFileIO;

SlicedImage *DAT_800d22dc;
uchar DAT_800d22e8;
int DAT_800d22ec;
int DAT_800d22f0;

void Pack_DefragmentFully();
void Mess_SetRGB(uchar, uchar, uchar, int);
void Mess_SetFontScale(uint);
void Mess_SetCurrentFont(char *);
int Mess_TextHeight(const char *);

int Front_BoardSelect1P(int player) {
	int x[3][2];
	int y[3][2];
	int idx[3][2];
	SlicedImage *img[3][2];
	int xoff[3];
	int yoff[3];
	int boxX[3];
	int boxY[3];
	int boxW[3];
	int boxH[3];
	Sprite *box[3];
	Sprite *bar[5];
	Sprite *row[5];
	int base[5];
	int board[5];
	int delta[5];
	int deck;
	int trucks;
	int n;
	int col;
	int i;

	FrontLoader_LoadSet(9);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("select_b.bmp"));
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	Pack_DefragmentFully();
	Mess_SetRGB(128, 128, 128, 0);
	Mess_SetFontScale(256);
	Mess_SetTextJustify(1);

	x[0][1] = x[0][0] = 144;
	y[0][0] = 117;
	y[0][1] = 397;
	idx[0][0] = GCareer.LastDeck[0];
	idx[0][1] = GCareer.LastDeck[0] + 1;

	if (idx[0][1] == DAT_800d33c4[0])
		idx[0][1] = 0;

	img[0][1] = img[0][0] = 0;
	xoff[0] = 31;
	yoff[0] = 12;
	boxX[0] = 144;
	boxY[0] = 117;
	boxW[0] = 127;
	boxH[0] = 280;

	x[1][1] = x[1][0] = 289;
	y[1][0] = 261;
	y[1][1] = 395;
	idx[1][0] = GCareer.LastTrucks[0];
	idx[1][1] = GCareer.LastTrucks[0] + 1;

	if (idx[1][1] == DAT_800d33c4[1])
		idx[1][1] = 0;

	img[1][1] = img[1][0] = 0;
	xoff[1] = 13;
	yoff[1] = 17;
	boxX[1] = 289;
	boxY[1] = 261;
	boxW[1] = 155;
	boxH[1] = 134;

	x[2][1] = x[2][0] = 429;
	y[2][0] = 273;
	y[2][1] = 383;
	idx[2][0] = GCareer.LastWheel[0];
	idx[2][1] = GCareer.LastWheel[0] + 1;

	if (idx[2][1] == DAT_800d33c4[2])
		idx[2][1] = 0;

	img[2][1] = img[2][0] = 0;
	xoff[2] = 27;
	yoff[2] = 23;
	boxX[2] = 429;
	boxY[2] = 273;
	boxW[2] = 118;
	boxH[2] = 110;

	DAT_800d22f0 = player;
	deck = idx[0][0];
	trucks = idx[1][0];

	box[0] = new Sprite(144, 117, 127, 280, 0, 64, 96);
	box[1] = new Sprite(289, 261, 155, 134, 0, 64, 62);
	box[2] = new Sprite(429, 273, 118, 110, 0, 64, 62);

	for (i = 0; i < 5; i++) {
		char name[16];

		if (i < 4)
			sprintf(name, "bar%d.bmp", i + 1);
		else
			sprintf(name, "bar9.bmp");

		bar[i] = new Sprite(name, 1, 32, 128);
		row[i] = new Sprite(398, i * 24 + 116, 200, 20, 1, 32, 128);
	}

	bar[4]->prepare();

	n = 0;
	base[4] = 0;
	base[0] = Stats[DAT_800d22f0].Stats[0];
	base[1] = Stats[DAT_800d22f0].Stats[1];
	base[2] = Stats[DAT_800d22f0].Stats[2];
	base[3] = Stats[DAT_800d22f0].Stats[3];

	while (n < 7) {
		if (GCareer.CharacterProgress[DAT_800d22f0].NumTapes <
		    Stats[DAT_800d22f0].Boards[n + 1].Req)
			break;

		n++;

		if (n >= 7)
			break;
	}

	board[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
	board[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
	board[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
	board[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];
	board[4] = DAT_800be438[trucks];
	delta[0] = 0;
	delta[1] = 0;
	delta[2] = 0;
	delta[3] = 0;
	delta[4] = 0;
	int statsDone = 0;
	int statsTimer = 2;
	int state = 1;
	int timer = 3;

	col = 0;
	int dir = 0;
	int speed = 0;
	int textState = 3;
	int textTimer = 2;
	int savedTextState = 2;
	int savedTextTimer = 10;

	char text[48];

	strcpy(text, Stats[DAT_800d22f0].Boards[deck].Name);

	Sprite *textBar = new Sprite(0, 49, 585, 43, 1, 32, 128);

	Mess_SetCurrentFont("mainf.fnt");

	int textX = -Mess_TextWidth(text);
	int textY;

	{
		int h = Mess_TextHeight(text);

		textY = (h >> 1) + 71 + Mess_TextHeightBelowBaseline(text);
	}

	int textSpeed = (117 - textX) / 10;

	textX = 117 - textSpeed * 10;

	Sprite *title[3];

	title[0] = new Sprite("Box1_01.bmp", 1, 32, 128);
	title[1] = new Sprite("Box1_02.bmp", 1, 32, 128);
	title[2] = new Sprite("Box1_03.bmp", 1, 32, 128);
	PadBuffer *input = new PadBuffer(-1);
	int result;

	FrontLoader_SetFiles(3);
	Front_InitBoardPictures1P();

	while (1) {
		int again;

		Front_UpdateMusicLoop();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		PollHost();

		if (DAT_800d22ec) {
			DAT_800d22dc->unpack();
			FrontLoader_Continue();
		} else {
			Front_StreamBoardPictures1P();
		}

		HiRes_ClearOTag();
		ResetOT();

		{
			int c = col;
			int k;

			for (k = 0; k < 3; k++) {
				c++;

				if (c >= 3)
					c = 0;

				if (c != col) {
					int j;

					if (state == 1)
						box[c]->draw(boxX[c], boxY[c], 0, -1, -1, -1);

					j = y[c][0] >= y[c][1];

					if (img[c][j] == 0) {
						SlicedImage *im = DAT_800d36a4[c][idx[c][j]];

						if (im) {
							img[c][j] = im;
							im->pack();
						}
					}

					if (img[c][j]) {
						if (state == 1)
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                1);
						else
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                0);
					} else if (c == 0) {
						DAT_800d22dc->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
						                   0);
					}
				} else {
					int j;

					title[c]->draw(boxX[c], boxY[c], 0, -1, -1, -1);
					SetClippingBox(boxX[c] + 6, boxY[c] + 9, boxW[c] - 12, boxH[c] - 18);

					for (j = 0; j < 2; j++) {
						if (img[c][j] == 0) {
							SlicedImage *im = DAT_800d36a4[c][idx[c][j]];

							if (im) {
								img[c][j] = im;
								im->pack();
							}
						}

						if (img[c][j])
							img[c][j]->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1, 0,
							                0);
						else if (c == 0)
							DAT_800d22dc->draw(x[c][j] + xoff[c], y[c][j] + yoff[c], 0, -1, -1, -1,
							                   0, 0);
					}

					SetClippingBox(0, 0, 640, 480);
				}
			}
		}

		if (textState != 3 && textState != 1) {
			textBar->draw(0, 49, 0, -1, -1, -1);
			Mess_SetCurrentFont("mainf.fnt");
			Mess_DrawText(textX, textY, text, 0, 0, 256, 0);
		}

		int ry = 116;

		if (!statsDone) {
			int r;

			for (r = 0; r < 5; r++) {
				int bx;

				row[r]->draw(398, ry, 0, -1, -1, -1);

				bx = 398;

				for (int b = 0; b < base[r]; b++) {
					bar[r & 3]->draw(bx, ry, 0, -1, -1, -1);
					bx += 15;
				}

				for (int b = 0; b < board[r]; b++) {
					bar[4]->draw(bx, ry, 0, -1, -1, -1);

					bx += 15;
				}

				for (int b = 0; b < delta[r]; b++) {
					bar[4]->drawFaded(bx, ry, 0, 64, 64, 64);

					bx += 15;
				}

				ry += 24;
			}
		}

		HiRes_DrawOTag();
		input->update();

		do {
			again = 0;

			switch (state) {
				case 0:
					if (DAT_800d22e8 > 1) {
						DAT_800d22e8--;
					} else {
						int press = input->getPress();

						if (press)
							DAT_800d22e8 = 3;

						if (press == 1) {
							SFX_PlayX(144, 0x2000, 0);

							timer = 4;
							state = 1;
							dir = -1;
							again = 1;
						} else if (press == 2) {
							SFX_PlayX(144, 0x2000, 0);

							timer = 4;
							state = 1;
							dir = 1;
							again = 1;
						} else if (press == 3) {
							int k;

							if (col == 0)
								SFX_PlayX(149, 0x2000, 0);
							else if (col == 1)
								SFX_PlayX(150, 0x2000, 0);
							else if (col == 2)
								SFX_PlayX(151, 0x2000, 0);

							timer = 10;
							speed = boxH[col] / 10;
							k = y[col][0] <= y[col][1];
							y[col][k] = boxY[col] - boxH[col];
							state = 2;
							idx[col][k] -= 2;

							if (idx[col][k] < 0)
								idx[col][k] += DAT_800d33c4[col];

							if (img[col][k])
								img[col][k]->unpack();

							img[col][k] = 0;
						} else if (press == 4) {
							if (col == 0)
								SFX_PlayX(149, 0x2000, 0);
							else if (col == 1)
								SFX_PlayX(150, 0x2000, 0);
							else if (col == 2)
								SFX_PlayX(151, 0x2000, 0);

							timer = 10;
							state = 2;
							speed = -boxH[col] / 10;
						}
					}

					break;
				case 1:
					if (--timer == 3) {
						col += dir;

						if (col < 0)
							col = 2;

						if (col >= 3)
							col = 0;

						savedTextState = textState;
						savedTextTimer = textTimer;
						textState = 3;
						textTimer = 2;
					}

					if (timer == 0) {
						state = 0;
						again = 1;
					}

					break;
				case 2:
					if (timer == 10 && col == 0) {
						textState = 4;
						textTimer = 1;
					}

					{
						int j;

						for (j = 0; j < 2; j++)
							y[col][j] += speed;
					}

					if (--timer == 0) {
						int k = y[col][0] >= y[col][1];

						if (speed < 0) {
							y[col][k] = boxY[col] + boxH[col];
							idx[col][k] += 2;

							if (idx[col][k] >= DAT_800d33c4[col])
								idx[col][k] -= DAT_800d33c4[col];

							if (img[col][k])
								img[col][k]->unpack();

							img[col][k] = 0;
							k = 1 - k;
							y[col][k] = boxY[col];

							if (col == 0)
								deck++;

							if (deck >= 8)
								deck = 0;

							if (col == 1)
								trucks++;

							if (trucks >= 3)
								trucks = 0;
						} else {
							y[col][k] = boxY[col];
							y[col][1 - k] = boxY[col] + boxH[col];

							if (col == 0)
								deck--;

							if (deck < 0)
								deck = 7;

							if (col == 1)
								trucks--;

							if (trucks < 0)
								trucks = 2;
						}

						again = 1;
						state = 0;

						if (col == 1)
							statsDone = 0;
					}

					break;
			}
		} while (again);

		switch (textState) {
			case 2:
				textTimer--;
				textX += textSpeed;

				if (textTimer == 0) {
					textState = 0;
					textTimer = 2;
				}

				break;
			case 3:
				if (--textTimer == 0) {
					textState = savedTextState;
					textTimer = savedTextTimer;
				}

				break;
			case 4:
				if (--textTimer == 0) {
					textSpeed = -textSpeed;
					textState = 5;
					textTimer = 10;
				}

				break;
			case 5:
				textTimer--;
				textX += textSpeed;

				if (textTimer == 0) {
					if (GCareer.CharacterProgress[DAT_800d22f0].NumTapes >=
					    Stats[DAT_800d22f0].Boards[deck].Req) {
						strcpy(text, Stats[DAT_800d22f0].Boards[deck].Name);
					} else if (GGame == 1) {
						sprintf(text, "Get %d tapes to unlock",
						        Stats[DAT_800d22f0].Boards[deck].Req);
					} else {
						sprintf(text, "Unlock in career mode",
						        Stats[DAT_800d22f0].Boards[deck].Req);
					}

					Mess_SetCurrentFont("mainf.fnt");

					textX = -Mess_TextWidth(text);

					{
						int h = Mess_TextHeight(text);

						textY = (h >> 1) + 71 + Mess_TextHeightBelowBaseline(text);
					}

					textState = 2;
					textTimer = 10;
					textSpeed = (117 - textX) / 10;
					textX = 117 - textSpeed * 10;
				}

				break;
			case 0:
				if (--textTimer == 0) {
					textState = 1;
					statsDone = 0;
					statsTimer = 3;
				}

				break;
		}

		if (!statsDone) {
			board[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
			board[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
			board[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
			board[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];

			if (n < deck) {
				delta[0] = Stats[DAT_800d22f0].Boards[deck].Stats[0] - board[0];
				delta[1] = Stats[DAT_800d22f0].Boards[deck].Stats[1] - board[1];
				delta[2] = Stats[DAT_800d22f0].Boards[deck].Stats[2] - board[2];
				delta[3] = Stats[DAT_800d22f0].Boards[deck].Stats[3] - board[3];
			} else {
				delta[0] = 0;
				delta[1] = 0;
				delta[2] = 0;
				delta[3] = 0;
			}

			board[4] = DAT_800be438[trucks];

			if (--statsTimer == 0)
				statsDone = 1;
		}

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			result = 1;
			GBoardStats[0].s[0] = Stats[DAT_800d22f0].Boards[n].Stats[0];
			GBoardStats[0].s[1] = Stats[DAT_800d22f0].Boards[n].Stats[1];
			GBoardStats[0].s[2] = Stats[DAT_800d22f0].Boards[n].Stats[2];
			GBoardStats[0].s[3] = Stats[DAT_800d22f0].Boards[n].Stats[3];
			GBoardStats[0].s[4] = DAT_800be438[trucks];

			if (GCareer.CharacterProgress[DAT_800d22f0].NumTapes >=
			    Stats[DAT_800d22f0].Boards[deck].Req) {
				int j;

				/* Two nested one-pass loops (macro-like): they double the refs of the
				   y/idx temps so local-alloc gives the y[c][1] loads the short-lived
				   registers first, and the loop note before the call frees sched2. */
				do {
					do {
						SFX_PlayX(145, 0x2000, 0);

						j = y[0][0] >= y[0][1];
						GCareer.LastDeck[0] = idx[0][j];
						j = y[1][0] >= y[1][1];
						GCareer.LastTrucks[0] = idx[1][j];
						j = y[2][0] >= y[2][1];
						GCareer.LastWheel[0] = idx[2][j];
					} while (0);
				} while (0);

				break;
			}

			SFX_PlayX(146, 0x2000, 0);
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(147, 0x2000, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			result = -1;
			break;
		}
	}

	HiRes_BlankScreen();
	SpoolerBlocksFileIO->unloadPRE("Equip1.pre");

	delete input;
	delete textBar;

	for (i = 0; i < 3; i++) {
		delete box[i];
		delete title[i];
	}

	for (i = 0; i < 5; i++) {
		delete bar[i];
		delete row[i];
	}

	Front_FreeBoardPictures1P();
	FrontLoader_Defragment();
	Pack_DefragmentFully();

	return result;
}
