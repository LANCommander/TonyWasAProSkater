/* Front_MemCardScreen__Fv (0x80060ba0) */
/* C++ (front end, THPS1-only; no demo twin). Runs the memory-card screen modally: sets up the
   card menus and a 640-byte block buffer, then loops the front-end frame (Front_MemCardHiResDisplay
   + Front_MemCardUpdate) until the update stops returning 18 (stay). Afterwards it clears the
   pads, plays the back sound, wipes both buffers, frees the menu and the buffer, reloads the
   options, clamps the volumes and copies the pad mapping. Returns -1 (39: cancelled), 19 (35)
   or 0. Called by FUN_800601fc and FUN_8005e8e0.
   Codegen: the exit code is inside the for(;;) as `if (r != 18) { ...; break; }`, which is why
   retail hoists &Pad and the 35 into s-registers before the loop. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/career.h"
#include "game/video.h"
#include "game/pad.h"
#include "game/shellutils.h"
#include "game/mem.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/rfront.h"

enum EMemCardState { MEMCARD_STATE_15 = 15 };
enum EPausedStatus { PAUSED_STATUS_39 = 39 };

extern SCareer GCareer;
extern SControl Pad[2];
extern SVideoInfo GVInfo1;
extern SVideoRestart GVideoRestart;
extern SOptions GPreferences;
extern SPadMap DAT_800bf260[2];
extern VerticalMenu *pVertMenu;

int DAT_800d2358;
int DAT_800d235c;

int Mess_GetFontScale();
void Mess_SetFontScale(uint);
void Pack_DefragmentFully();
int SFX_PlayX(uint, short, int);
void Front_InitMemCardMenus(int, EMemCardState, EPausedStatus);
void Front_MemCardHiResDisplay();
int Front_MemCardUpdate();
void Front_RestoreBackground();

int Front_MemCardScreen() {
	int scale;
	int r;
	int ret;
	int i;

	scale = Mess_GetFontScale();
	GCareer.Version = 0xdeadbeef;
	Pack_DefragmentFully();
	Front_InitMemCardMenus(640, MEMCARD_STATE_15, PAUSED_STATUS_39);
	Front_SetLoadAction(LOAD_GAME);

	GVInfo1.pBlockStart = (ulong *)Mem_NewX(640, 0, 1);
	GVideoRestart.NumSkaters = 0;

	for (;;) {
		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		HiRes_ClearOTag();
		ResetOT();

		if (pVertMenu)
			pVertMenu->setTriangles((Sprite *)DAT_800d2358, (Sprite *)DAT_800d235c);

		Front_MemCardHiResDisplay();
		HiRes_DrawOTag();

		r = Front_MemCardUpdate();

		if (r != 18) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(145, 8192, 0);
			Mess_SetFontScale(scale);

			if (r == 39)
				ret = -1;
			else if (r == 35)
				ret = 19;
			else
				ret = 0;

			break;
		}
	}

	for (i = 0; i < 2; i++) {
		HiRes_ClearOTag();
		ResetOT();
		Front_RestoreBackground();

		if (pVertMenu)
			pVertMenu->erase();

		HiRes_DrawOTag();
	}

	DrawSync(0);
	Front_FreeBackground();

	if (pVertMenu)
		Front_FreeMemCardMenus();

	Mem_Delete(GVInfo1.pBlockStart);
	Options_GetPreferences();

	if (GPreferences.SFXVolume > 12)
		GPreferences.SFXVolume = 12;

	if (GPreferences.MusicVolume > 12)
		GPreferences.MusicVolume = 12;

	for (int p = 0; p < 2; p++) {
		DAT_800bf260[p].Function[0] = GPreferences.ControllerFunction[p][0];
		DAT_800bf260[p].Function[2] = GPreferences.ControllerFunction[p][2];
		DAT_800bf260[p].Function[1] = GPreferences.ControllerFunction[p][1];
		DAT_800bf260[p].Function[3] = GPreferences.ControllerFunction[p][3];
	}

	return ret;
}
