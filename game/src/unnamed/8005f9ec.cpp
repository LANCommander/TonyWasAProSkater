/* Front_SoundOptionsHandler__FP12VerticalMenu (0x8005f9ec) */
#include "game/shellutils.h"
#include "game/pad.h"
#include "game/vector.h"
#include "game/career.h"
#include "game/sfx.h"
#include "game/gamefmv.h"

extern SOptions GPreferences;
extern SControl Pad[2];
extern int SFXLEVEL;
extern int XALEVEL;
extern short GameFMV_StartX;

extern Sprite *DAT_800d1d5c;
extern Sprite *DAT_800d1d60;

void GameFMV_StopFMV(void);
void GameFMV_StartFMV(int);

void Front_SoundOptionsHandler(VerticalMenu *a) {
	int v, w, u, i, dir;

	XALEVEL = a->getSelected() == 1 ? GPreferences.MusicVolume * 0x555 : 0;
	SFXLEVEL = GPreferences.SFXVolume * 0x555;

	GameFMV_SyncFMV(0);

	if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered || Pad[0].MenuRight.Triggered ||
	    Pad[1].MenuRight.Triggered) {
		dir = 0;

		if (Pad[0].MenuLeft.Triggered || Pad[1].MenuLeft.Triggered)
			dir = 1;

		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		if (a->getSelected() == 0) {
			w = GPreferences.SFXVolume + 1;

			if (dir)
				w = GPreferences.SFXVolume - 1;

			GPreferences.SFXVolume = w;

			if (w < 0)
				GPreferences.SFXVolume = 0;

			v = GPreferences.SFXVolume;

			if (v >= 12) {
				GPreferences.SFXVolume = 12;
				v = 12;
			}

			SFXLEVEL = v * 0x555;
			SFX_PlayX(145, 8192, 0);
		}

		if (a->getSelected() == 1) {
			u = GPreferences.MusicVolume + 1;

			if (dir)
				u = GPreferences.MusicVolume - 1;

			GPreferences.MusicVolume = u;

			if (u < 0)
				GPreferences.MusicVolume = 0;

			if (GPreferences.MusicVolume >= 12)
				GPreferences.MusicVolume = 12;
		}
	}

	if (Pad[0].MenuStart.Triggered || Pad[1].MenuStart.Triggered || Pad[0].MenuAccept.Triggered ||
	    Pad[1].MenuAccept.Triggered) {
		if (a->getSelected() == 0) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			SFX_PlayX(145, 8192, 0);
		}

		if (a->getSelected() == 1) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
			GameFMV_StopFMV();
			CVector t(0, 0, 0);

			GameFMV_SetStartTrack(2);
			GameFMV_AddSoundSource(&t);

			GameFMV_StartX = -1;
			GameFMV_StartFMV(0);
		}
	}

	for (i = 0; i < 12; i++) {
		v = 32;

		if (i < GPreferences.SFXVolume)
			v = 128;

		DAT_800d1d5c->draw(i * 24 + 206, 204, 0, v, v, v);

		v = 32;

		if (i < GPreferences.MusicVolume)
			v = 128;

		DAT_800d1d60->draw(i * 24 + 206, 284, 0, v, v, v);
	}
}
