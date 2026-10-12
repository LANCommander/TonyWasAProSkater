/* FUN_8005d3dc (0x8005d3dc) */
/* C++ (front end, THPS1-only; no demo twin). Loading-screen frame for the picture screens
   (DAT_800d1cf8 = 1, 3, 4, 5; dispatched by FUN_8005d370). Called once per load callback while
   loading (DAT_800d1cec = 0) and, after FUN_8005dc8c sets DAT_800d1cec, it loops until the player
   continues: draws the background, the level picture (DAT_800d1d0c) and the blinking "Press X"
   sprite (DAT_800d1d04, toggled by the DAT_800d1d08 counter). START/X or GAutoTest continue
   (sound 6, fade 64); triangle backs out (sound 2). Afterwards it deletes the two sprites. */
#include "game/shellutils.h"
#include "game/pad.h"

extern SControl Pad[2];
extern int DAT_800bf0d4[]; /* picture x by screen */
extern int DAT_800bf0e0[]; /* picture y by screen */
extern int GAutoTest;
extern uint GameFade; /* GameFade */
extern int DAT_800d1cb8;  /* START+SELECT quit */

int DAT_800d1ce8;
int DAT_800d1cec;     /* loading finished */
int BackOut;     /* backed out */
int DAT_800d1cf8;     /* screen type */
Sprite *DAT_800d1d04; /* "Press X" */
int DAT_800d1d08;     /* blink counter */
Sprite *DAT_800d1d0c; /* level picture */
int DAT_800d1d14;     /* redraw picture */

int SFX_PlayX(uint, short, int);
void Pause(int);

extern "C" {
	void FUN_8008eebc(int);
	int DrawSync(int);
	void FUN_8006b2f4(void);
	void FUN_8006b240(void);
	void FUN_80061ea0(void);
	void FUN_800242cc(void);
	void DrawColoredBox__FiiiiUcUcUc(int, int, int, int, int, int, int);
	void SetClippingBox__Fiiii(int, int, int, int);
	void FUN_80070ff4(void);
	void FUN_8005c68c(void);
}

extern "C" void FUN_8005d3dc(void) {
	do {
		if (DAT_800d1cec)
			FUN_8008eebc(0);
		DrawSync(0);
		FUN_8006b2f4();
		FUN_8006b240();
		FUN_80061ea0();
		FUN_800242cc();
		if (BackOut) {
			DrawColoredBox__FiiiiUcUcUc(0, 0, 512, 240, 0, 0, 0);
			return;
		}
		SetClippingBox__Fiiii(0, 0, 512, 240);
		if (DAT_800d1d14) {
			DAT_800d1d14 = 0;
			if (DAT_800d1cf8 == 4)
				DAT_800d1d0c->draw(DAT_800bf0d4[1], DAT_800bf0e0[1], 0, -1, -1, -1);
			else
				DAT_800d1d0c->draw(DAT_800bf0d4[2], DAT_800bf0e0[2], 0, -1, -1, -1);
		}
		if (DAT_800d1cec) {
			if (--DAT_800d1d08 == 0)
				DAT_800d1d08 = 92;
			if (DAT_800d1d08 == 91) {
				if (DAT_800d1cf8 == 4)
					DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
				else
					DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
			} else if (DAT_800d1d08 == 15) {
				if (DAT_800d1cf8 == 4)
					DAT_800d1d0c->draw(DAT_800bf0d4[1], DAT_800bf0e0[1], 0, -1, -1, -1);
				else
					DAT_800d1d0c->draw(DAT_800bf0d4[2], DAT_800bf0e0[2], 0, -1, -1, -1);
			}
		}
		FUN_80070ff4();
		FUN_8005c68c();
		if (DAT_800d1cb8) {
			DAT_800d1ce8 = 0;
			BackOut = 1;
			break;
		}
		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered || GAutoTest) {
			SFX_PlayX(6, 8192, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			GameFade = 64;
			break;
		}
		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(2, 8192, 0);
			Pause(20);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			DAT_800d1ce8 = 0;
			BackOut = 1;
		}
	} while (DAT_800d1cec);

	if (DAT_800d1cec) {
		delete DAT_800d1d04;
		delete DAT_800d1d0c;
	}
}
