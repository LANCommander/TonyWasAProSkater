/* LoadingScreen_WaitLevel__Fv (0x8005d73c) */
/* C++ (front end, THPS1-only; no demo twin). Loading-screen frame for the level loading screen
   (DAT_800d1cf8 = 0, set up by FUN_8005d00c; dispatched by FUN_8005d370). The sibling of
   LoadingScreen_Wait (loadscreenwait.cpp): same loop, but there is no background clear, the picture
   always sits at the screen-0 position, the menu input DAT_800bf104 is updated each frame,
   continuing (START/X) also sets DAT_800d1d80, triangle only eats its own triggers, and the
   tape-marker sprite DAT_800d1d00 is deleted too. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/shellutils.h"
#include "game/pad.h"
#include "game/psyq.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/main.h"
#include "pc_frame.h"

extern SControl Pad[2];
extern PadBuffer DAT_800bf104;
extern int DAT_800bf0d4[]; /* picture x by screen */
extern int DAT_800bf0e0[]; /* picture y by screen */
extern int DAT_800d1d80;
extern uint GameFade;    /* GameFade */
extern int DAT_800d1cb8; /* START+SELECT quit */

int DAT_800d1ce8;
int DAT_800d1cec;     /* loading finished */
int BackOut;          /* backed out */
int DAT_800d1cf8;     /* screen type */
Sprite *DAT_800d1d00; /* got-tape marker */
Sprite *DAT_800d1d04; /* "Press X" */
int DAT_800d1d08;     /* blink counter */
Sprite *DAT_800d1d0c; /* level picture */
int DAT_800d1d14;     /* redraw picture */

int SFX_PlayX(uint, short, int);
void Pause(int);

void LoadingScreen_WaitLevel(void) {
	do {
		if (DAT_800d1cec)
			PC_FrameWait(1);

		DrawSync(0);
		HiRes_DrawOTagNoSync();
		HiRes_ClearOTag();
		ResetOT();
		PollHost();

		if (BackOut) {
			DrawColoredBox(0, 0, 512, 240, 0, 0, 0);
			return;
		}

		if (DAT_800d1d14) {
			DAT_800d1d14 = 0;
			DAT_800d1d0c->draw(DAT_800bf0d4[0], DAT_800bf0e0[0], 0, -1, -1, -1);
		}

		if (DAT_800d1cec) {
			if (--DAT_800d1d08 == 0)
				DAT_800d1d08 = 92;

			if (DAT_800d1d08 == 91)
				DAT_800d1d04->draw(88, 196, 0, -1, -1, -1);
			else if (DAT_800d1d08 == 15)
				DAT_800d1d0c->draw(DAT_800bf0d4[0], DAT_800bf0e0[0], 0, -1, -1, -1);
		}

		DAT_800bf104.update();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			DAT_800d1ce8 = 0;
			BackOut = 1;
			break;
		}

		if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered ||
		    Pad[0].MenuAccept.Triggered || Pad[1].MenuAccept.Triggered) {
			SFX_PlayX(6, 8192, 0);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			GameFade = 64;
			DAT_800d1d80 = 1;
			break;
		}

		if (Pad[0].MenuBack.Triggered || Pad[1].MenuBack.Triggered) {
			SFX_PlayX(2, 8192, 0);
			Pause(20);

			Pad[0].MenuBack.Triggered = 0;
			Pad[1].MenuBack.Triggered = 0;
			DAT_800d1ce8 = 0;
			BackOut = 1;
		}
	} while (DAT_800d1cec);

	if (DAT_800d1cec) {
		delete DAT_800d1d04;
		delete DAT_800d1d0c;
		delete DAT_800d1d00;
	}
}
