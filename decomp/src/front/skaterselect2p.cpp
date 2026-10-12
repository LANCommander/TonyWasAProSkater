/* FUN_80056754 (0x80056754) flags=-fstructure-noalias */
/* C++ (front end, 2-player skater select). Each player spins a CBruce model of the highlighted
   skater on a turntable inside a frame; left/right slide the model out and the next skater in
   (models stream in through the credits spooler, "LOADING" is shown until the PSX is resident),
   the skater's name slides in on a text bar, and after the name settles an info panel (six text
   lines) and four stat bars are drawn. X/Start locks a skater in ("skater chosen"), triangle
   backs out. Returns player 1's skater (player 2's goes to SelectedSkater2), or -1. No demo twin.
   Levers (as in the 1P sibling): the function-wide `i` is reused as the counter of the stat
   bars' inner loop and of the teardown loops, which lowers its global-alloc priority so the
   counters land in s1 under the pointers/offsets; the best-board scan is a top-tested
   `while (best < 7) { if (tapes < Req) break; best++; }`. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
}

#include "game/shellutils.h"
#include "game/career.h"
#include "game/pad.h"
#include "game/front.h"
#include "game/bruce.h"
#include "game/m3d.h"

extern SControl Pad[];
extern SCareer GCareer;
extern SStats Stats[];
extern char DoubleBuffer[];
extern char MikeCamera[];
extern char *pDoubleBuffer;
extern char *MechList;
extern int DAT_800d3758[];
extern int DAT_800d3788[];

static int DAT_800d2304[2];
static int DAT_800d231c[2];
static int DAT_800d2300;
static int DAT_800d22fc;
static int DAT_800d2314;

extern int DAT_800d1cb8;
extern int SelectedSkater2;
extern int Cheat_BigHead;
extern int Loading;
extern int GHeadPart;
extern int GHeadUp;

int DAT_800d1c1c = 32;
int DAT_800d1c20 = 0;
int DAT_800d1c24 = 0;

typedef char SLabel7[7] __attribute__((aligned(4)));

void Pack_DefragmentFully();
void Mess_SetRGB(uchar, uchar, uchar, int);
void Mess_SetFontScale(uint);
void Mess_SetCurrentFont(char *);
int Mess_TextHeight(const char *);
void Mess_LoadFont(char *, int, int, int);
void Mem_Copy(void *, void *, int);
void M3d_BuildTransform(CSuper *);

extern "C" {
	void FUN_8005c8dc(int);
	void FUN_8005c97c(int);
	void FUN_8005ca64(void);
	void *FUN_8005ca3c(char *);
	void FUN_80061714(void *);
	void FUN_80059520(int);
	int FUN_8008b900(char *, const char *, ...);
	void FUN_8008c190(const char *, ...);
	void FUN_8008c468(int);
	void FUN_80055eb8(void);
	SViewport *FUN_8006b1c8(void);
	void FUN_8006b240(void);
	void FUN_8005c780(void);
	void FUN_80070ff4(void);
	void FUN_8005c68c(void);
	void FUN_800242cc(void);
	void FUN_80055f80(void);
	void FUN_8005c934(void);
	void FUN_80061ea0(void);
	void FUN_80059618(int);
	void FUN_80043f70(CBruce *);
	int SFX_PlayX__FUisi(int, int, int);
	void FUN_8006ed2c(int);
	void ProcessNewPSX__Fi(int);
	CBruce *FUN_8005622c(int);
	void FUN_80056428(void);
	void FUN_800565ac(void);
	void FUN_80061eb4(int);
	void FUN_80093240(SVECTOR *, MATRIX *);
	void M3d_RenderSetup__FP7SCameraP9SViewportPUi(char *, SViewport *, char *);
	void M3d_Render__FPv(char *);
	void FUN_80083774(void);
	void Mess_SetTextJustify__FUc(int);
	int Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
	int Mess_TextHeightAboveBaseline__FPCc(char *);
	int Mess_TextWidth__FPCc(char *);
	void SetClippingBox__Fiiii(int, int, int, int);
	void FUN_80059018(void);
	void FUN_8008c8dc(char *);
	void FUN_80056394(void);
	void FUN_8006b374(void);
	void FUN_8006af64(void);
	void FileIO_Sync__Fv(void);
	void FUN_8005609c(void);
	void FUN_80059758(void);
}

extern "C" int FUN_80056754(void) {
	char spare[40]; /* never referenced; retail's frame has these 40 bytes at sp+0x20 */
	char name[16];
	Sprite *bars[4];
	Sprite *buttons[4];
	int leftX[2];
	int rightX[2];
	int leftTimer[2];
	int rightTimer[2];
	SStatBar stat[2][4];
	int best[2];
	Sprite *info[3][2];
	int panelX[2];
	int panelY[2];
	Sprite *panel[2];
	Sprite *textBar[2];
	Sprite *frame[2];
	int minX[2];
	int maxX[2];
	int homeX[2];
	SVECTOR rot[2];
	CBruce *bruce[2];
	int state[2];
	int speed[2];
	int shown[2];
	int result;
	int numSkaters;
	SViewport *vp;
	int i;
	int p;
	void *buf;
	void *bmp;

	FUN_8005c8dc(2);
	FUN_8005c97c(2);
	FUN_8005ca64();

	buf = Mem_NewX(0x26160, 1, 1);
	bmp = FUN_8005ca3c("select2p.bmp");

	Mem_Copy(buf, bmp, 0x26160);
	Mem_Delete(bmp);
	Mess_LoadFont("stats2.fnt", -1, -1, -1);
	Mess_LoadFont("mainf.fnt", -1, -1, -1);
	Mess_SetFontScale(200);
	FUN_80061714(buf);

	result = 0;
	DAT_800d1c20 = 0;
	numSkaters = 10;

	for (i = 0; i < 11; i++) {
		if (GCareer.CharacterProgress[i].NumTapes >= 30) {
			DAT_800d1c20 = 1;
			numSkaters = 11;
		}
	}

	for (int k = 0; k < 2; k++) {
		DAT_800d2304[k] = GCareer.LastSkater[k];

		if (!DAT_800d1c24 && DAT_800d1c20 && DAT_800d2304[1 - k] != 10)
			DAT_800d2304[k] = 10;

		if (DAT_800d2304[k] == 10 && !DAT_800d1c20)
			DAT_800d2304[k] = k;
	}

	DAT_800d1c24 = DAT_800d1c20;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	if (DAT_800d2304[0] == DAT_800d2304[1]) {
		DAT_800d2304[1] = DAT_800d2304[0] + 1;

		if (DAT_800d2304[1] >= numSkaters)
			DAT_800d2304[1] = 0;
	}

	Pack_DefragmentFully();
	FUN_80059520(2);

	for (i = 0; i < 4; i++) {
		FUN_8008b900(name, "bar%d.bmp", i + 5);

		bars[i] = new Sprite(name, 1, 32, 128);
		bars[i]->removeFromMemory();
	}

	for (i = 0; i < 4; i++) {
		FUN_8008b900(name, "levbut%d.bmp", i + 1);

		buttons[i] = new Sprite(name, 1, 32, 128);
		buttons[i]->removeFromMemory();
	}

	leftX[0] = 98;
	leftX[1] = 386;
	rightX[0] = 271;
	rightX[1] = 559;
	leftTimer[0] = leftTimer[1] = 0;
	rightTimer[0] = rightTimer[1] = 0;

	for (int p = 0; p < 2; p++) {
		int k;

		for (k = 0; k < 4; k++) {
			if (p == 0)
				stat[p][k].x = 203;
			else
				stat[p][k].x = 476;

			stat[p][k].y = k * 17 + 256;
			stat[p][k].type = k;
		}

		stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
		stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
		stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
		stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
	}

	info[0][0] = new Sprite(203, 323, 150, 30, 1, 32, 128);

	info[0][0]->removeFromMemory();

	info[0][1] = info[0][0];
	info[1][0] = new Sprite(203, 352, 165, 18, 1, 32, 128);

	info[1][0]->removeFromMemory();

	info[1][1] = info[1][0];
	info[2][0] = new Sprite(203, 369, 90, 47, 1, 32, 128);

	info[2][0]->removeFromMemory();

	info[2][1] = info[2][0];

	for (int p = 0; p < 2; p++) {
		if (p == 0)
			panelX[p] = 203;
		else
			panelX[p] = 476;

		panelY[p] = 248;
	}

	panel[0] = new Sprite(panelX[0], panelY[0], 160, 90, 1, 32, 128);

	panel[0]->removeFromMemory();

	panel[1] = panel[0];
	textBar[0] = new Sprite(0, 50, 340, 41, 1, 32, 128);

	textBar[0]->removeFromMemory();

	textBar[1] = new Sprite(340, 50, 300, 41, 1, 32, 128);

	textBar[1]->removeFromMemory();

	frame[0] = new Sprite(144, 92, 129, 142, 1, 16, 32);

	frame[0]->removeFromMemory();

	frame[1] = frame[0];

	minX[0] = DAT_800d2300 - DAT_800d1c1c * 0x7000;
	minX[1] = DAT_800d22fc - DAT_800d1c1c * 0x7000;
	maxX[0] = DAT_800d2300 + DAT_800d1c1c * 0x7000;
	maxX[1] = DAT_800d22fc + DAT_800d1c1c * 0x7000;
	homeX[0] = DAT_800d2300;
	homeX[1] = DAT_800d22fc;

	FUN_8008c468(0);
	FUN_80055eb8();
	FUN_8008c468(0);

	rot[0].vx = rot[0].vy = rot[0].vz = 0;
	rot[1].vx = rot[1].vy = rot[1].vz = 0;
	vp = FUN_8006b1c8();
	vp->xR = 640;
	vp->yB = 480;
	bruce[0] = 0;
	bruce[1] = 0;

	for (int j = 0; j < 2; j++) {
		state[j] = 0;
		speed[j] = DAT_800d1c1c;
		shown[i] = 1; /* sic: i (== 4) from the sprite loop, not j */
	}

	SLabel7 sSkater = "skater";
	int pad1f0[2]; /* unreferenced 8-byte gaps around the labels in retail's frame */
	SLabel7 sChosen = "chosen";
	int pad200[2];
	int lx[2];
	int ly[2];
	PadBuffer *input[2];
	int panelState[2];
	int infoTimer[2];
	int unused[2];
	int infoX[2];
	int frameX[2];
	int barX[2];
	int textX[2];
	int textY[2];
	int startX[2];
	int endX[2];
	int textSpeed[2];
	int textState[2];
	int loadX[2];
	lx[0] = 208;
	lx[1] = 496;
	ly[0] = 153;
	ly[1] = 173;
	input[0] = new PadBuffer(0);
	input[1] = new PadBuffer(1);

	for (int j = 0; j < 2; j++) {
		panelState[j] = 0;
		infoTimer[j] = 0;
		unused[j] = 0;
	}

	infoX[0] = 203;
	infoX[1] = 476;
	frameX[0] = 144;
	frameX[1] = 432;
	barX[0] = 0;
	barX[1] = 340;
	textState[0] = 0;
	textState[1] = 0;
	pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? DoubleBuffer + 32940 : DoubleBuffer;

	FUN_8006b240();

	pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? DoubleBuffer + 32940 : DoubleBuffer;

	FUN_8006b240();

	loadX[0] = frameX[0] + 64;
	loadX[1] = frameX[1] + 64;

	FUN_8005c8dc(10);

	for (;;) {
		FUN_8005c780();
		FUN_80070ff4();
		FUN_8005c68c();

		if (DAT_800d1cb8)
			goto out;

		FUN_800242cc();

		if (!DAT_800d2314)
			FUN_80055f80();
		else
			FUN_8005c934();

		FUN_8006b240();
		FUN_80061ea0();

		for (p = 0; p < 2; p++) {
			int done;
			int press;

			FUN_80059618(0);
			input[p]->update();

			if (DAT_800d231c[p]) {
				FUN_80043f70(bruce[p]);
				FUN_80043f70(bruce[p]);
			}

			done = 0;

			switch (state[p]) {
				case 0:
				default:
					rot[p].vy += 10;
					press = input[p]->getPress();

					if (press == 2) {
						SFX_PlayX__FUisi(148, 0x2000, 0);

						state[p] = 2;
						textState[p] = 3;
						rightTimer[p] = 16;
					} else if (press == 1) {
						SFX_PlayX__FUisi(148, 0x2000, 0);

						state[p] = 3;
						textState[p] = 3;
						leftTimer[p] = 16;
					}

					if (!DAT_800d231c[p] && DAT_800d3788[DAT_800d2304[p]]) {
						if (DAT_800d2304[0] == DAT_800d2304[1] && DAT_800d231c[1 - p]) {
							DAT_800d231c[p] = 1;
						} else {
							DAT_800d231c[p] = 1;
							FUN_8006ed2c(DAT_800d3758[DAT_800d2304[p]]);

							if (Cheat_BigHead) {
								GHeadUp = Stats[DAT_800d2304[p]].Neck;
								GHeadPart = Stats[DAT_800d2304[p]].Head;
							}

							ProcessNewPSX__Fi(DAT_800d3758[DAT_800d2304[p]]);

							GHeadPart = -1;
						}

						bruce[p] = FUN_8005622c(p);
						bruce[p]->mPos.vx = homeX[p];
					}

					break;
				case 1:
					input[p]->reset();

					if (DAT_800d231c[p]) {
						while (rot[p].vy > 2048)
							rot[p].vy -= 4096;

						while (rot[p].vy < -2048)
							rot[p].vy += 4096;

						if (rot[p].vy > -80 && rot[p].vy < 80)
							rot[p].vy = 0;
						else if (rot[p].vy < 0)
							rot[p].vy += 80;
						else if (rot[p].vy > 0)
							rot[p].vy -= 80;

						if (bruce[p]->mPos.vx > homeX[p] - (DAT_800d1c1c << 13) &&
						    bruce[p]->mPos.vx < homeX[p] + (DAT_800d1c1c << 13))
							bruce[p]->mPos.vx = homeX[p];
						else if (bruce[p]->mPos.vx < homeX[p])
							bruce[p]->mPos.vx += DAT_800d1c1c << 13;
						else if (bruce[p]->mPos.vx > homeX[p])
							bruce[p]->mPos.vx -= DAT_800d1c1c << 13;
					}

					break;
				case 2:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx -= speed[p] << 12;

						if (bruce[p]->mPos.vx < minX[p])
							state[p] = 5;
					} else {
						loadX[p] -= 12;

						if (loadX[p] < frameX[p])
							state[p] = 5;
					}

					break;
				case 3:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx += speed[p] << 12;

						if (bruce[p]->mPos.vx >= maxX[p])
							state[p] = 4;
					} else {
						loadX[p] += 12;

						if (loadX[p] >= frameX[p] + 129)
							state[p] = 4;
					}

					break;
				case 5:
					shown[1 - p] = 0;
					state[p] = 7;
					break;
				case 7:
					state[p] = 9;
					FUN_80056428();

					if (DAT_800d231c[p]) {
						if (bruce[p]) {
							delete bruce[p];
							bruce[p] = 0;
						}
					}

					if (++DAT_800d2304[p] >= numSkaters)
						DAT_800d2304[p] = 0;

					DAT_800d231c[p] = DAT_800d3788[DAT_800d2304[p]];
					FUN_800565ac();

					if (DAT_800d231c[p]) {
						if (bruce[p])
							FUN_8008c190("\nOH FUCK!!!");

						bruce[p] = FUN_8005622c(p);
						bruce[p]->mPos.vx = maxX[p];
					}

					loadX[p] = frameX[p] + 129;
					textState[p] = 0;
					stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
					stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
					stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
					stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
					break;
				case 4:
					shown[1 - p] = 0;
					state[p] = 6;
					break;
				case 6:
					state[p] = 8;
					FUN_80056428();

					if (DAT_800d231c[p]) {
						if (bruce[p]) {
							delete bruce[p];
							bruce[p] = 0;
						}
					}

					if (--DAT_800d2304[p] < 0)
						DAT_800d2304[p] = numSkaters - 1;

					DAT_800d231c[p] = DAT_800d3788[DAT_800d2304[p]];
					FUN_800565ac();

					if (DAT_800d231c[p]) {
						if (bruce[p])
							FUN_8008c190("\nOH FUCK!!!");

						bruce[p] = FUN_8005622c(p);
						bruce[p]->mPos.vx = minX[p];
					}

					loadX[p] = frameX[p];
					textState[p] = 0;
					stat[p][0].value = Stats[DAT_800d2304[p]].Stats[0];
					stat[p][1].value = Stats[DAT_800d2304[p]].Stats[1];
					stat[p][2].value = Stats[DAT_800d2304[p]].Stats[2];
					stat[p][3].value = Stats[DAT_800d2304[p]].Stats[3];
					break;
				case 8:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx += speed[p] << 12;

						if (bruce[p]->mPos.vx >= homeX[p]) {
							done = 1;
							bruce[p]->mPos.vx = homeX[p];
						}
					} else {
						loadX[p] += 12;

						if (loadX[p] >= frameX[p] + 64) {
							loadX[p] = frameX[p] + 64;
							done = 1;
						}
					}

					if (done) {
						state[p] = 0;
						press = input[p]->getPress();

						if (press == 2) {
							SFX_PlayX__FUisi(148, 0x2000, 0);

							state[p] = 2;
							textState[p] = 3;
							rightTimer[p] = 16;
						} else if (press == 1) {
							SFX_PlayX__FUisi(148, 0x2000, 0);

							state[p] = 3;
							textState[p] = 3;
							leftTimer[p] = 16;
						}
					}

					break;
				case 9:
					if (DAT_800d231c[p]) {
						bruce[p]->mPos.vx -= speed[p] << 12;

						if (bruce[p]->mPos.vx <= homeX[p]) {
							bruce[p]->mPos.vx = homeX[p];
							done = 1;
						}
					} else {
						loadX[p] -= 12;

						if (loadX[p] <= frameX[p] + 64) {
							loadX[p] = frameX[p] + 64;
							done = 1;
						}
					}

					if (done) {
						state[p] = 0;
						press = input[p]->getPress();

						if (press == 2) {
							SFX_PlayX__FUisi(148, 0x2000, 0);

							state[p] = 2;
							textState[p] = 3;
							rightTimer[p] = 16;
						} else if (press == 1) {
							SFX_PlayX__FUisi(148, 0x2000, 0);

							state[p] = 3;
							textState[p] = 3;
							leftTimer[p] = 16;
						}
					}

					break;
			}

			FUN_80061eb4(p);

			if (DAT_800d231c[p]) {
				FUN_80093240(&rot[p], &bruce[p]->mRotMatrix);

				bruce[p]->mRotMatrix.m[0][1] = -bruce[p]->mRotMatrix.m[0][1];
				bruce[p]->mRotMatrix.m[1][1] = -bruce[p]->mRotMatrix.m[1][1];
				bruce[p]->mRotMatrix.m[2][1] = -bruce[p]->mRotMatrix.m[2][1];

				if (shown[p])
					frame[p]->draw(frameX[p], 92, 0, -1, -1, -1);

				if (bruce[p]->mPos.vx >= minX[p] && maxX[p] >= bruce[p]->mPos.vx && shown[p]) {
					bruce[p ^ 1]->mFlags ^= 0x41;
					M3d_BuildTransform(bruce[p]);

					bruce[0]->mShadowLights = 0;
					bruce[1]->mShadowLights = 0;
					M3d_RenderSetup__FP7SCameraP9SViewportPUi(MikeCamera, vp, pDoubleBuffer + ((p << 14) + 140));
					M3d_Render__FPv(MechList);
					FUN_80083774();

					bruce[p ^ 1]->mFlags ^= 0x41;
				} else if (!shown[p]) {
					shown[p] = 1;
				}
			} else if (state[p] == 0) {
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify__FUc(0);
				Mess_DrawText__FiiPCciUiib(loadX[p], 163, "LOADING", 0, 0, 256, 0);
			}

			if (state[p] == 1) {
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(250, 190, 60, 0);
				Mess_SetTextJustify__FUc(0);
				Mess_DrawText__FiiPCciUiib(lx[p], ly[0], sSkater, 1, 0, 256, 0);
				Mess_DrawText__FiiPCciUiib(lx[p], ly[1], sChosen, 1, 0, 256, 0);
			}

			FUN_80061eb4(p);

			if (infoTimer[p]) {
				infoTimer[p]--;
				info[0][p]->draw(infoX[p], 323, 0, -1, -1, -1);
				info[1][p]->draw(infoX[p], 352, 0, -1, -1, -1);
				info[2][p]->draw(infoX[p], 369, 0, -1, -1, -1);
				Mess_SetCurrentFont("stats2.fnt");
				Mess_SetRGB(128, 128, 128, 0);
				Mess_SetTextJustify__FUc(1);
				int ty = 330;

				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[0]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[0]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[0], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[1]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[1]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[1], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[2]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[2]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[2], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[3]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[3]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[3], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[4]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[4]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[4], 0, 0, 256, 0);

				ty += 15;
				Mess_DrawText__FiiPCciUiib(infoX[p],
				             Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].pBio[5]) + ty -
				                 (Mess_TextHeight(Stats[DAT_800d2304[p]].pBio[5]) >> 1),
				             Stats[DAT_800d2304[p]].pBio[5], 0, 0, 256, 0);
			}

			if (panelState[p] == -1 && panelState[1 - p] <= 0 && infoTimer[1 - p] <= 0 &&
			    textState[1 - p] == 1) {
				panelState[p] = 2;
			} else if (panelState[p] > 0) {
				int k;

				if (--panelState[p] == 0)
					infoTimer[p] = 2;

				panel[p]->draw(panelX[p], panelY[p], 0, -1, -1, -1);

				best[p] = 0;

				while (best[p] < 7) {
					if (GCareer.CharacterProgress[DAT_800d2304[p]].NumTapes <
					    Stats[DAT_800d2304[p]].Boards[best[p] + 1].Req)
						break;

					best[p]++;
				}

				for (k = 0; k < 4; k++) {
					int x = stat[p][k].x;
					int type = stat[p][k].type;
					int y = stat[p][k].y - (bars[type]->m_h >> 1);

					for (i = 0; i < stat[p][k].value; i++) {
						bars[type]->draw(x, y, 0, -1, -1, -1);

						x += 14;
					}
				}
			}

			if (leftTimer[p]) {
				if (--leftTimer[p] == 0)
					buttons[0]->draw(leftX[p], 95, 0, -1, -1, -1);
				else
					buttons[2]->draw(leftX[p], 95, 0, -1, -1, -1);
			}

			if (rightTimer[p]) {
				if (--rightTimer[p] == 0)
					buttons[1]->draw(rightX[p], 95, 0, -1, -1, -1);
				else
					buttons[3]->draw(rightX[p], 95, 0, -1, -1, -1);
			}

			switch (textState[p]) {
				case 1:
					break;
				case 0:
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify__FUc(1);

					if (p == 0) {
						startX[p] = textX[p] = -Mess_TextWidth__FPCc(Stats[DAT_800d2304[p]].Name);
						endX[p] = 208 - (Mess_TextWidth__FPCc(Stats[DAT_800d2304[p]].Name) >> 1);
					} else {
						startX[p] = textX[p] = 640;
						endX[p] = 496 - (Mess_TextWidth__FPCc(Stats[DAT_800d2304[p]].Name) >> 1);
					}

					textSpeed[p] = (endX[p] - startX[p]) / 6;
					startX[p] -= textSpeed[p];
					endX[p] -= textSpeed[p];

					{
						int ty = 74;

						textY[p] = Mess_TextHeightAboveBaseline__FPCc(Stats[DAT_800d2304[p]].Name) + ty -
						           (Mess_TextHeight(Stats[DAT_800d2304[p]].Name) >> 1);
					}

					textState[p] = 2;
					break;
				case 2:
					textX[p] += textSpeed[p];

					if ((textX[p] < startX[p] && textX[p] < endX[p]) ||
					    (textX[p] > startX[p] && textX[p] > endX[p])) {
						panelState[p] = -1;
						textState[p] = 1;
					}

					textBar[p]->draw(barX[p], 50, 0, -1, -1, -1);
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify__FUc(1);
					Mess_DrawText__FiiPCciUiib(textX[p], textY[p], Stats[DAT_800d2304[p]].Name, 0, 0, 256,
					             0);
					break;
				case 3:
					textX[p] -= textSpeed[p];

					if ((textX[p] < startX[p] && textX[p] < endX[p]) ||
					    (textX[p] > startX[p] && textX[p] > endX[p]))
						textState[p] = 1;

					textBar[p]->draw(barX[p], 50, 0, -1, -1, -1);
					Mess_SetCurrentFont("mainf.fnt");
					Mess_SetRGB(128, 128, 128, 0);
					Mess_SetTextJustify__FUc(1);
					Mess_DrawText__FiiPCciUiib(textX[p], textY[p], Stats[DAT_800d2304[p]].Name, 0, 0, 256,
					             0);
					break;
			}

			FUN_80061eb4(p);

			if (DAT_800d231c[p])
				SetClippingBox__Fiiii(frameX[p], 92, 129, 142);
			else
				SetClippingBox__Fiiii(0, 0, 640, 480);

			if (p == 0) {
				FUN_80059018();

				pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? DoubleBuffer + 32940 : DoubleBuffer;
				FUN_8008c8dc(pDoubleBuffer + 32904);

				pDoubleBuffer = pDoubleBuffer == DoubleBuffer ? DoubleBuffer + 32940 : DoubleBuffer;
			}
		}

		FUN_80056394();
		FUN_80059018();
		FUN_8006b374();

		for (p = 0; p < 2; p++) {
			if (Pad[p].Start.Triggered || Pad[p].X.Triggered) {
				Pad[p].Start.Triggered = 0;
				Pad[p].X.Triggered = 0;

				if (state[p] != 1 && DAT_800d231c[p] &&
				    (DAT_800d2304[0] != DAT_800d2304[1] || state[1 - p] != 1)) {
					state[p] = 1;
					SFX_PlayX__FUisi(145, 0x2000, 0);
				} else if (state[p] != 1 && DAT_800d231c[p]) {
					SFX_PlayX__FUisi(146, 0x2000, 0);
				}
			}

			if (Pad[p].Triangle.Triggered) {
				Pad[p].Triangle.Triggered = 0;

				if (state[p] != 1) {
					if (state[1 - p] != 1) {
						SFX_PlayX__FUisi(147, 0x2000, 0);

						result = -1;
						break;
					}
				} else {
					SFX_PlayX__FUisi(147, 0x2000, 0);

					state[p] = 0;
				}
			}
		}

		if (result == -1)
			goto out;

		if (state[0] == 1 && state[1] == 1) {
			result = DAT_800d2304[0];

			for (int k = 0; k < 2; k++) {
				if (GCareer.LastSkater[k] != DAT_800d2304[k]) {
					GCareer.LastSkater[k] = 0;
					GCareer.LastDeck[k] = 0;
					GCareer.LastWheel[k] = 0;
					GCareer.LastTrucks[k] = 1;
					GCareer.LastLevel = 0;
				}

				GCareer.LastSkater[k] = DAT_800d2304[k];
			}

			goto out;
		}
	}

out:

	FUN_8006af64();
	if (Loading)
		FileIO_Sync__Fv();

	for (i = 0; i < 2; i++) {
		if (DAT_800d231c[i]) {
			delete bruce[i];
			bruce[i] = 0;
		}
	}

	FUN_8005609c();
	FUN_80059758();

	for (i = 0; i < 4; i++)
		delete bars[i];
	for (int k = 0; k < 4; k++)
		delete buttons[k];
	delete info[0][0];
	delete info[1][0];
	delete info[2][0];
	delete panel[0];
	delete frame[0];

	for (i = 0; i < 2; i++) {
		delete textBar[i];
		delete input[i];
	}

	SelectedSkater2 = DAT_800d2304[1];
	return result;
}
