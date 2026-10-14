/* Game_Logic__Fv (0x80024868) flags=-fstructure-noalias */
/* C++ (MAIN.CPP). Fuzzy demo twin; locals (Frozen) from MAIN.SYM. Runs the per-frame game logic
   on every other frame: objects, tricks, panels, front end and messages, swapping the two pads
   around the front-end update in two-player mode. */
#include "game/bit.h"
#include "game/bruce.h"
#include "game/camera.h"
#include "game/fileio.h"
#include "game/flash.h"
#include "game/font.h"
#include "game/front.h"
#include "game/mess.h"
#include "game/ob.h"
#include "game/pad.h"
#include "game/panel.h"
#include "game/shatter.h"
#include "game/spool.h"
#include "game/trickob.h"
#include "game/tricktips.h"
#include "game/utils.h"

extern "C" {
	extern SControl Pad[2];
	extern int GamePaused[1];

	int EveryOther;
	int DAT_800d183c;
	int TTime;
	int GameFrozen;
	int GGame;
	int GState;
	int GSpin;
	int GSpinSpeed;
	int GHorsePlayer;
	CBruce *GSkater;
	CBruce *DAT_800d17cc;

	extern CBody *BaddyList;
	extern CBody *EnvironmentalObjectList;
	extern CBody *MechList;
	extern CBody *PowerUpList;
	extern CBody *BackgroundList;
	extern CBody *CameraList;
}

void Game_Logic(void) {
	int Frozen;

	DAT_800d183c++;

	if (EveryOther++ & 1)
		return;

	TTime++;
	Font_Reset();

	Frozen = 0;

	if (GamePaused[0] || GameFrozen)
		Frozen = 1;

	if (!Frozen) {
		Flash_Update();
		Utils_ClearExplosionStack();
		Ob_ClearInputFlags(BaddyList);
		Ob_ClearInputFlags(EnvironmentalObjectList);
		Ob_AI(&MechList);

		if (!GamePaused[0]) {
			Bit_Move();
			Rail_Update();
			Ob_AI(&PowerUpList);
			TrickOb_Update();
			Ob_QuickAI(&EnvironmentalObjectList);
			Ob_QuickAI(&BackgroundList);
			Ob_MaybeUnSuspendOrCull();
			Ob_AI(&BaddyList);
			Bit_RemoveDeadBits();

			if (GGame != 7)
				if (GState == 1 && GSpin < 40 && !GSkater->mPhysicsState &&
				    (!DAT_800d17cc || !DAT_800d17cc->mPhysicsState)) {
					GSkater->mpCamera->extra_YRot += GSpinSpeed;

					if (DAT_800d17cc)
						DAT_800d17cc->mpCamera->extra_YRot += GSpinSpeed;
				}
		}
	} else {
		if (GState == 1)
			Ob_AI(&CameraList);
	}

	Shatter_MaybeMakeGlassShatterSound();
	Panel_Update();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	Pad_Remap();
	Front_Update();
	Pad_Restore();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	TrickTips_Update();
	Mess_Update();
	Spool_Continue();
	FileIO_Continue();
}
