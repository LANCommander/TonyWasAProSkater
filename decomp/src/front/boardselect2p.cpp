/* FUN_80051660 (0x80051660) */
/* C++ (front end, 2-player board select). The 2-player twin of FUN_80054510 (boardselect1p.cpp):
   every piece of per-screen state is an array indexed by player. Each player has three columns
   (deck, trucks, wheels) scrolling pairs of SlicedImage pictures through a clipped box, a sliding
   text bar with the deck's name (or "Get %d tapes"), and four stat rows of bar sprites. X/Start
   locks in an unlocked deck ("board chosen" pops up over a saved screen rectangle), triangle
   backs out. Returns 1 once both players have chosen, -1 if both back out. No demo twin. */
#include "game/shellutils.h"
#include "game/pre.h"
#include "game/career.h"
#include "game/pad.h"

extern SControl Pad[];
extern SCareer GCareer;
extern SStats Stats[];
extern BoardStats GBoardStats[];
extern int DAT_800be42c[];
extern int DAT_800d2c1c[];
extern SlicedImage *DAT_800d31cc[][3][15];
extern int DAT_800d1cb8;
extern int SelectedSkater2;
extern PREManager *SpoolerBlocksFileIO;
extern char s_select2b_bmp[];
extern char s_bar_d_bmp[];
extern char s_bar9_bmp[];
extern char s_mainf_fnt[];
extern char s_Box2_01_bmp[];
extern char s_Box2_02_bmp[];
extern char s_Box2_03_bmp[];
extern char s_Get_d_tapes[];
extern char s_Equip2_pre[];

/* per-player screen layout (player 0, player 1) */
int DAT_800d1bac[2] = { 0, 368 };   /* text bar x */
int DAT_800d1bb4[2] = { 368, 272 }; /* text bar width */
int DAT_800d1bbc[2] = { 92, 374 };  /* text slide target x */
int DAT_800d1bc4[2] = { 188, 460 }; /* stat rows x */
int DAT_800d1bcc[2] = { 135, 423 }; /* deck column x */
int DAT_800d1bd4[2] = { 172, 460 };
int DAT_800d1bdc[2] = { 199, 487 }; /* trucks column x */
int DAT_800d1be4[2] = { 236, 524 }; /* "board chosen" x */
int DAT_800d1bec[2] = { 265, 553 }; /* wheels column x */
int DAT_800d1bf4[2] = { 294, 582 };
static SlicedImage *DAT_800d22b0;
static uchar DAT_800d22c8;
static int DAT_800d22cc;
static int DAT_800d22d0[2];

/* word-aligned label buffers (copied with lw/lh/lb) */
typedef char SLabel6[6] __attribute__((aligned(4)));
typedef char SLabel7[7] __attribute__((aligned(4)));

void Pack_DefragmentFully();
void Mess_SetRGB(uchar, uchar, uchar, int);
void Mess_SetFontScale(uint);
void Mess_SetCurrentFont(char *);
int Mess_TextHeight(const char *);

extern "C" {
	void FUN_8005c97c(int);
	void FUN_8005ca64(void);
	int FUN_8005ca3c(char *);
	void FUN_80061714(int);
	void Mess_SetTextJustify__FUc(int);
	int FUN_8008b900(char *, const char *, ...);
	void FUN_8008e904(char *, char *);
	int Mess_TextWidth__FPCc(char *);
	int Mess_TextHeightBelowBaseline__FPCc(const char *);
	void Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
	void SFX_PlayX__FUisi(int, int, int);
	void FUN_8005c8dc(int);
	void FUN_80050c5c(void);
	void FUN_8005c780(void);
	void FUN_80070ff4(void);
	void FUN_8005c68c(void);
	void FUN_800242cc(void);
	void FUN_8005c934(void);
	void FUN_8005115c(void);
	void FUN_8006b240(void);
	void FUN_80061ea0(void);
	void SetClippingBox__Fiiii(int, int, int, int);
	void FUN_8006b26c(void);
	void FUN_8006af64(void);
	void FUN_800514f0(void);
}

extern "C" int FUN_80051660(int player) {
	int x[2][3][2];
	int y[2][3][2];
	int idx[2][3][2];
	SlicedImage *img[2][3][2];
	int xoff[2][3];
	int yoff[2][3];
	int boxX[2][3];
	int boxY[2][3];
	int boxW[2][3];
	int boxH[2][3];
	int deck[2];
	int trucks[2];
	Sprite *box[3];
	Sprite *bar[5];
	Sprite *row[5];
	int base[2][5];
	int statsDone[2];
	int statsTimer[2];
	int i;

	FUN_8005c97c(10);
	FUN_8005ca64();
	FUN_80061714(FUN_8005ca3c(s_select2b_bmp));
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	Pack_DefragmentFully();

	for (int p = 0; p < 2; p++) {
		x[p][0][0] = DAT_800d1bcc[p];
		x[p][0][1] = DAT_800d1bcc[p];
		y[p][0][0] = 119;
		y[p][0][1] = 265;
		idx[p][0][0] = GCareer.LastDeck[p];
		idx[p][0][1] = GCareer.LastDeck[p] + 1;

		if (idx[p][0][1] == DAT_800d2c1c[0])
			idx[p][0][1] = 0;

		img[p][0][0] = 0;
		img[p][0][1] = 0;
		xoff[p][0] = 21;
		yoff[p][0] = 9;
		boxX[p][0] = DAT_800d1bcc[p];
		boxY[p][0] = 119;
		boxW[p][0] = 74;
		boxH[p][0] = 146;

		x[p][1][0] = DAT_800d1bdc[p];
		x[p][1][1] = DAT_800d1bdc[p];
		y[p][1][0] = 190;
		y[p][1][1] = 254;
		idx[p][1][0] = GCareer.LastTrucks[p];
		idx[p][1][1] = GCareer.LastTrucks[p] + 1;

		if (idx[p][1][1] == DAT_800d2c1c[1])
			idx[p][1][1] = 0;

		img[p][1][0] = 0;
		img[p][1][1] = 0;
		xoff[p][1] = 8;
		yoff[p][1] = 8;
		boxX[p][1] = DAT_800d1bdc[p];
		boxY[p][1] = 190;
		boxW[p][1] = 76;
		boxH[p][1] = 64;

		x[p][2][0] = DAT_800d1bec[p];
		x[p][2][1] = DAT_800d1bec[p];
		y[p][2][0] = 209;
		y[p][2][1] = 265;
		idx[p][2][0] = GCareer.LastWheel[p];
		idx[p][2][1] = GCareer.LastWheel[p] + 1;

		if (idx[p][2][1] == DAT_800d2c1c[2])
			idx[p][2][1] = 0;

		img[p][2][0] = 0;
		img[p][2][1] = 0;
		xoff[p][2] = 16;
		yoff[p][2] = 12;
		boxX[p][2] = DAT_800d1bec[p];
		boxY[p][2] = 209;
		boxW[p][2] = 64;
		boxH[p][2] = 56;
	}

	DAT_800d22d0[0] = player;
	DAT_800d22d0[1] = SelectedSkater2;
	deck[0] = idx[0][0][0];
	deck[1] = idx[1][0][0];
	trucks[0] = idx[0][1][0];
	trucks[1] = idx[1][1][0];

	box[0] = new Sprite(DAT_800d1bcc[0], 119, 74, 146, 0, 64, 96);
	box[1] = new Sprite(DAT_800d1bdc[0], 190, 76, 64, 0, 64, 62);
	box[2] = new Sprite(DAT_800d1bec[0], 209, 64, 56, 0, 64, 62);

	for (i = 0; i < 5; i++) {
		char name[16];

		if (i < 4)
			FUN_8008b900(name, s_bar_d_bmp, i + 1);
		else
			FUN_8008b900(name, s_bar9_bmp);

		bar[i] = new Sprite(name, 1, 32, 128);
		row[i] = new Sprite(DAT_800d1bc4[0], i * 22 + 294, 200, 20, 1, 32, 128);
	}

	bar[4]->prepare();

	/* best unlocked board per player (computed but unused in 2P). The loop bound in a variable
	   set outside the player loop keeps cse1 from folding the entry test; retail's
	   `li t9,1; beq t9,zero` guard is the PRE copy of it, rematerialized after spilling. */
	int n[2];
	int lastBoard = 7;

	for (int p = 0; p < 2; p++) {
		base[p][0] = Stats[DAT_800d22d0[p]].Stats[0];
		base[p][1] = Stats[DAT_800d22d0[p]].Stats[1];
		base[p][2] = Stats[DAT_800d22d0[p]].Stats[2];
		base[p][3] = Stats[DAT_800d22d0[p]].Stats[3];
		base[p][4] = 0;
		n[p] = 0;

		while (n[p] < lastBoard) {
			if (GCareer.CharacterProgress[DAT_800d22d0[p]].NumTapes <
			    Stats[DAT_800d22d0[p]].Boards[n[p] + 1].Req)
				break;

			n[p]++;

			if (n[p] >= 7)
				break;
		}

		base[p][4] = DAT_800be42c[trucks[p]];
		statsDone[p] = 0;
		statsTimer[p] = 2;
	}

	int state[2];
	int timer[2];
	int col[2];
	int dir[2];
	int speed[2];
	int chosen[2];

	for (int p = 0; p < 2; p++) {
		state[p] = 1;
		timer[p] = 3;
		col[p] = 0;
		dir[p] = 0;
		speed[p] = 0;
		chosen[p] = 0;
	}

	SLabel6 sBoard = "board";
	SLabel7 sChosen = "chosen";
	int lx[2];
	int ly[2];
	EraseSprite *popup[2];
	lx[0] = DAT_800d1be4[0];
	lx[1] = DAT_800d1be4[1];
	ly[0] = 180;
	ly[1] = 200;
	popup[0] = 0;
	popup[1] = 0;

	Mess_SetCurrentFont(s_mainf_fnt);
	Mess_SetFontScale(200);
	Mess_SetTextJustify__FUc(1);

	int textState[2];
	int textTimer[2];
	int savedTextState[2];
	int savedTextTimer[2];
	char text[2][48];
	int textX[2];
	int textY[2];
	int textSpeed[2];
	Sprite *textBar[2];

	for (int p = 0; p < 2; p++) {
		textState[p] = 3;
		textTimer[p] = 2;
		savedTextState[p] = 2;
		savedTextTimer[p] = 10;
		FUN_8008e904(text[p], Stats[DAT_800d22d0[p]].Boards[deck[p]].Name);

		if (p == 0)
			textX[p] = -Mess_TextWidth__FPCc(text[0]);
		else
			textX[p] = 640;

		{
			int h = Mess_TextHeight(text[p]);

			textY[p] = (h >> 1) + 75 + Mess_TextHeightBelowBaseline__FPCc(text[p]);
		}

		textSpeed[p] = (DAT_800d1bbc[p] - textX[p]) / 10;
		textX[p] = DAT_800d1bbc[p] - textSpeed[p] * 10;
		textBar[p] = new Sprite(DAT_800d1bac[p], 50, DAT_800d1bb4[p], 41, 1, 32, 128);
	}

	Sprite *title[3];
	title[0] = new Sprite(s_Box2_01_bmp, 1, 32, 128);
	title[1] = new Sprite(s_Box2_02_bmp, 1, 32, 128);
	title[2] = new Sprite(s_Box2_03_bmp, 1, 32, 128);
	PadBuffer *input[2];
	input[0] = new PadBuffer(0);
	input[1] = new PadBuffer(1);

	FUN_8005c8dc(3);
	FUN_80050c5c();

	int result = 0;

	while (!result) {
		FUN_8005c780();
		FUN_80070ff4();
		FUN_8005c68c();

		if (DAT_800d1cb8)
			break;

		FUN_800242cc();

		if (DAT_800d22cc) {
			DAT_800d22b0->unpack();
			FUN_8005c934();
		} else {
			FUN_8005115c();
		}

		FUN_8006b240();
		FUN_80061ea0();

		for (int p = 0; p < 2; p++) {
			int c = col[p];
			int cnt = 0;
			int k;

			if (chosen[p] < 4)
				cnt = 3;

			for (k = 0; k < cnt; k++) {
				c++;

				if (c >= 3)
					c = 0;

				if (c != col[p]) {
					int j;

					if (state[p] == 1)
						box[c]->draw(boxX[p][c], boxY[p][c], 0, -1, -1, -1);

					j = y[p][c][0] >= y[p][c][1];

					if (img[p][c][j] == 0) {
						SlicedImage *im = DAT_800d31cc[p][c][idx[p][c][j]];

						if (im) {
							img[p][c][j] = im;
							im->pack();
						}
					}

					if (img[p][c][j]) {
						if (state[p] == 1)
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 1);
						else
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
					} else if (c == 0) {
						DAT_800d22b0->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0, -1,
						                   -1, -1, 0, 0);
					}
				} else {
					int j;

					title[c]->draw(boxX[p][c], boxY[p][c], 0, -1, -1, -1);
					SetClippingBox__Fiiii(boxX[p][c] + 3, boxY[p][c] + 5, boxW[p][c] - 6,
					                      boxH[p][c] - 10);

					for (j = 0; j < 2; j++) {
						if (img[p][c][j] == 0) {
							SlicedImage *im = DAT_800d31cc[p][c][idx[p][c][j]];

							if (im) {
								img[p][c][j] = im;
								im->pack();
							}
						}

						if (img[p][c][j])
							img[p][c][j]->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
						else if (c == 0)
							DAT_800d22b0->draw(x[p][c][j] + xoff[p][c], y[p][c][j] + yoff[p][c], 0,
							                   -1, -1, -1, 0, 0);
					}

					SetClippingBox__Fiiii(0, 0, 640, 480);
				}
			}

			if (textState[p] != 3 && textState[p] != 1) {
				textBar[p]->draw(DAT_800d1bac[p], 50, 0, -1, -1, -1);
				Mess_SetCurrentFont(s_mainf_fnt);
				Mess_SetFontScale(200);
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify__FUc(1);
				Mess_DrawText__FiiPCciUiib(textX[p], textY[p], text[p], 0, 0, 256, 0);
			}

			int ry = 294;

			if (!statsDone[p]) {
				int r;

				for (r = 0; r < 5; r++) {
					int bx;

					row[r]->draw(DAT_800d1bc4[p], ry, 0, -1, -1, -1);

					bx = DAT_800d1bc4[p];

					for (int b = 0; b < base[p][r]; b++) {
						bar[r]->draw(bx, ry, 0, -1, -1, -1);

						bx += 15;
					}

					ry += 22;
				}
			}

			if (chosen[p] == 4) {
				if (popup[p] == 0)
					popup[p] = new EraseSprite(DAT_800d1be4[p] - 40, 150, 80, 80);

				Mess_SetCurrentFont(s_mainf_fnt);
				Mess_SetFontScale(150);
				Mess_SetRGB(250, 190, 60, 0);
				Mess_SetTextJustify__FUc(0);
				Mess_DrawText__FiiPCciUiib(lx[p], ly[0], sBoard, 1, 0, 256, 0);
				Mess_DrawText__FiiPCciUiib(lx[p], ly[1], sChosen, 1, 0, 256, 0);
			}

			if (chosen[p] == 5) {
				popup[p]->restore(DAT_800d1be4[p] - 40, 150);
				delete popup[p];
				popup[p] = 0;
				chosen[p] = 0;
			}
		}

		FUN_8006b26c();

		for (int p = 0; p < 2; p++) {
			int again;

			input[p]->update();

			do {
				again = 0;

				switch (state[p]) {
					case 0:
						if (DAT_800d22c8 > 1) {
							DAT_800d22c8--;
						} else if (chosen[p] >= 4) {
							input[p]->reset();
						} else {
							if (chosen[p] > 0)
								chosen[p]++;

							int press = input[p]->getPress();

							if (press)
								DAT_800d22c8 = 3;

							if (press == 1) {
								SFX_PlayX__FUisi(144, 0x2000, 0);

								timer[p] = 4;
								state[p] = 1;
								dir[p] = -1;
								again = 1;
							} else if (press == 2) {
								SFX_PlayX__FUisi(144, 0x2000, 0);

								timer[p] = 4;
								state[p] = 1;
								dir[p] = 1;
								again = 1;
							} else if (press == 3) {
								int k;

								if (col[p] == 0)
									SFX_PlayX__FUisi(149, 0x2000, 0);
								else if (col[p] == 1)
									SFX_PlayX__FUisi(150, 0x2000, 0);
								else if (col[p] == 2)
									SFX_PlayX__FUisi(151, 0x2000, 0);

								timer[p] = 10;
								state[p] = 2;
								speed[p] = boxH[p][col[p]] / 10;
								k = y[p][col[p]][0] <= y[p][col[p]][1];
								y[p][col[p]][k] = boxY[p][col[p]] - boxH[p][col[p]];
								idx[p][col[p]][k] -= 2;

								if (idx[p][col[p]][k] < 0)
									idx[p][col[p]][k] += DAT_800d2c1c[col[p]];

								if (img[p][col[p]][k])
									img[p][col[p]][k]->unpack();

								img[p][col[p]][k] = 0;
							} else if (press == 4) {
								if (col[p] == 0)
									SFX_PlayX__FUisi(149, 0x2000, 0);
								else if (col[p] == 1)
									SFX_PlayX__FUisi(150, 0x2000, 0);
								else if (col[p] == 2)
									SFX_PlayX__FUisi(151, 0x2000, 0);

								timer[p] = 10;
								state[p] = 2;
								speed[p] = -boxH[p][col[p]] / 10;
							}
						}

						break;
					case 1:
						if (--timer[p] == 3) {
							col[p] += dir[p];

							if (col[p] < 0)
								col[p] = 2;

							if (col[p] >= 3)
								col[p] = 0;

							savedTextState[p] = textState[p];
							savedTextTimer[p] = textTimer[p];
							textState[p] = 3;
							textTimer[p] = 2;
						}

						if (timer[p] == 0) {
							again = 1;
							state[p] = 0;
						}

						break;
					case 2:
						if (timer[p] == 10 && col[p] == 0) {
							textState[p] = 4;
							textTimer[p] = 1;
						}

						{
							int j;

							for (j = 0; j < 2; j++)
								y[p][col[p]][j] += speed[p];
						}

						if (--timer[p] == 0) {
							int k = y[p][col[p]][0] >= y[p][col[p]][1];

							if (speed[p] < 0) {
								y[p][col[p]][k] = boxY[p][col[p]] + boxH[p][col[p]];
								idx[p][col[p]][k] += 2;

								if (idx[p][col[p]][k] >= DAT_800d2c1c[col[p]])
									idx[p][col[p]][k] -= DAT_800d2c1c[col[p]];

								if (img[p][col[p]][k])
									img[p][col[p]][k]->unpack();

								img[p][col[p]][k] = 0;
								k = 1 - k;
								y[p][col[p]][k] = boxY[p][col[p]];

								if (col[p] == 0)
									deck[p]++;

								if (deck[p] >= 8)
									deck[p] = 0;

								if (col[p] == 1)
									trucks[p]++;

								if (trucks[p] >= 3)
									trucks[p] = 0;
							} else {
								y[p][col[p]][k] = boxY[p][col[p]];
								y[p][col[p]][1 - k] = boxY[p][col[p]] + boxH[p][col[p]];

								if (col[p] == 0)
									deck[p]--;

								if (deck[p] < 0)
									deck[p] = 7;

								if (col[p] == 1)
									trucks[p]--;

								if (trucks[p] < 0)
									trucks[p] = 2;
							}

							state[p] = 0;
							again = 1;

							if (col[p] == 1)
								statsDone[p] = 0;
						}

						break;
				}
			} while (again);

			switch (textState[p]) {
				case 2:
					textTimer[p]--;
					textX[p] += textSpeed[p];

					if (textTimer[p] == 0) {
						textState[p] = 0;
						textTimer[p] = 2;
					}

					break;
				case 3:
					if (--textTimer[p] == 0) {
						textState[p] = savedTextState[p];
						textTimer[p] = savedTextTimer[p];
					}

					break;
				case 4:
					if (--textTimer[p] == 0) {
						textSpeed[p] = -textSpeed[p];
						textState[p] = 5;
						textTimer[p] = 10;
					}

					break;
				case 5:
					textTimer[p]--;
					textX[p] += textSpeed[p];

					if (textTimer[p] == 0) {
						if (GCareer.CharacterProgress[DAT_800d22d0[p]].NumTapes >=
						    Stats[DAT_800d22d0[p]].Boards[deck[p]].Req)
							FUN_8008e904(text[p], Stats[DAT_800d22d0[p]].Boards[deck[p]].Name);
						else
							FUN_8008b900(text[p], s_Get_d_tapes,
							             Stats[DAT_800d22d0[p]].Boards[deck[p]].Req);

						Mess_SetCurrentFont(s_mainf_fnt);

						if (p == 0)
							textX[p] = -Mess_TextWidth__FPCc(text[0]);
						else
							textX[p] = 640;

						{
							int h = Mess_TextHeight(text[p]);

							textY[p] = (h >> 1) + 75 + Mess_TextHeightBelowBaseline__FPCc(text[p]);
						}

						textSpeed[p] = (DAT_800d1bbc[p] - textX[p]) / 10;
						textX[p] = DAT_800d1bbc[p] - textSpeed[p] * 10;
						textState[p] = 2;
						textTimer[p] = 10;
					}

					break;
				case 0:
					if (--textTimer[p] == 0) {
						textState[p] = 1;
						statsDone[p] = 0;
						statsTimer[p] = 3;
					}

					break;
			}

			if (!statsDone[p]) {
				base[p][4] = DAT_800be42c[trucks[p]];

				if (--statsTimer[p] == 0)
					statsDone[p] = 1;
			}

			if (Pad[p].MenuStart.Triggered || Pad[p].MenuAccept.Triggered) {
				Pad_ClearTriggers(&Pad[p]);

				GBoardStats[p].s[0] = 3;
				GBoardStats[p].s[1] = 3;
				GBoardStats[p].s[2] = 3;
				GBoardStats[p].s[3] = 3;
				GBoardStats[p].s[4] = DAT_800be42c[trucks[p]];

				if (GCareer.CharacterProgress[DAT_800d22d0[p]].NumTapes >=
				    Stats[DAT_800d22d0[p]].Boards[deck[p]].Req) {
					int j;

					GCareer.LastDeck[p] = deck[p];
					j = y[p][1][0] >= y[p][1][1];
					GCareer.LastTrucks[p] = idx[p][1][j];
					j = y[p][2][0] >= y[p][2][1];
					GCareer.LastWheel[p] = idx[p][2][j];

					if (chosen[p] == 0) {
						if (speed[p] > 0 && col[p] == 0 && timer[p] != 0) {
							timer[p] = 10 - timer[p];
							speed[p] = -speed[p];

							if (textState[p] == 5)
								textTimer[p] += 8;

							if (--deck[p] < 0)
								deck[p] = 7;
						} else if (speed[p] < 0 && col[p] == 0 && timer[p] != 0) {
							timer[p] = 10 - timer[p];
							speed[p] = -speed[p];

							if (textState[p] == 5)
								textTimer[p] += 8;

							if (++deck[p] >= 8)
								deck[p] = 0;
						}

						SFX_PlayX__FUisi(145, 0x2000, 0);

						chosen[p] = 1;
					}
				} else {
					SFX_PlayX__FUisi(146, 0x2000, 0);
				}

				if (chosen[0] && chosen[1])
					result = 1;
			}

			if (Pad[p].MenuBack.Triggered) {
				if (chosen[p] || !chosen[1 - p])
					SFX_PlayX__FUisi(147, 0x2000, 0);

				Pad_ClearTriggers(&Pad[p]);

				if (!chosen[0] && !chosen[1])
					result = -1;

				if (chosen[p] == 4)
					chosen[p] = 5;
			}
		}
	}

	FUN_8006af64();
	SpoolerBlocksFileIO->unloadPRE(s_Equip2_pre);

	delete popup[0];
	delete popup[1];
	delete input[0];
	delete input[1];
	delete textBar[0];
	delete textBar[1];

	for (i = 0; i < 3; i++) {
		delete box[i];
		delete title[i];
	}

	for (i = 0; i < 5; i++) {
		delete bar[i];
		delete row[i];
	}

	FUN_800514f0();

	return result;
}
