/* CROWD.CPP: crowd reactions in competitions: cheers, "oww"s and "ahh"s scaled by score and speed
   (declarations in game/crowd.h). */

#include "game/crowd.h"
#include "game/career.h"
#include "game/psyq.h"
#include "game/sfx.h"
#include "game/utils.h"
#include "game/globals.h"

/* one crowd ambience voice: the looping SFX voice and its current/target L/R volumes */
typedef struct {
	int Voice;
	int VolL, VolR;
	int TargetL, TargetR;
	int pad;
} SCrowdVoice;
extern SCrowdVoice Crowd[];
extern int Crowd_Bored;
int Crowd_MoveTowards(int &, int, int);
extern "C" void FUN_80011060(int);
extern int DAT_800d10e8;

/* Crowd_Init__Fv (0x800109b0) */
void Crowd_Init(void) {}

/* Crowd_Start__Fv (0x800109b8) */
void Crowd_Start() {
	int i;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			Crowd_Bored = 0;

			for (i = 0; i < 1; i++) {
				Crowd[i].Voice = SFX_PlayX(SND_CROWDAMBIENCE, 0, 0);
				printf("Pitch = %d \n", SFX_GetVoicePitch(Crowd[i].Voice));

				Crowd[i].VolL = Crowd[i].TargetL = Crowd[i].TargetR = Crowd[i].VolR = 0;
			}
		}
	}
}

/* Crowd_Stop__Fv (0x80010ac0) */
void Crowd_Stop() {
	int i;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			for (i = 0; i < 1; i++) {
				if (Crowd[i].Voice)
					SFX_Stop(Crowd[i].Voice);

				Crowd[i].Voice = 0;
			}
		}
	}
}

/* Crowd_MoveTowards__FRiii (0x80010b58) */
int Crowd_MoveTowards(int &cur, int target, int speed) {
	if (cur == target)
		return 0;

	if (cur > target - speed && cur < target + speed)
		cur = target;
	else if (cur < target)
		cur += speed;
	else
		cur -= speed;

	return 1;
}

/* Crowd_Update__Fv (0x80010bb8) */
void Crowd_Update() {
	int i;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			for (i = 0; i <= 0; i++) {
				if (Crowd[i].Voice) {
					if (Crowd_MoveTowards(Crowd[i].VolL, Crowd[i].TargetL, 100) ||
					    Crowd_MoveTowards(Crowd[i].VolR, Crowd[i].TargetR, 100)) {
						SFX_ModifyVol(Crowd[i].Voice, (Crowd[i].VolL * 2) / 3, (Crowd[i].VolR * 2) / 3);
					} else {
						if (Rnd(30) == 0) {
							Crowd[i].TargetL = Rnd(8192);
							Crowd[i].TargetR = Rnd(8192);
						}
					}
				}
			}

			if (Crowd_Bored && Rnd(50) == 0) {
				FUN_80011060(Crowd_Bored << 13);

				Crowd_Bored--;
			}
		}
	}
}

/* Crowd_Cheer__Fi (0x80010d84) */
void Crowd_Cheer(int a) {
	int v;

	if (a) {
		if (Levels[GLevel].Crowd) {
			if (GGame == 1) {
				if (Crowd_Bored) {
					if (a >= 100)
						Crowd_Bored = 0;
					else
						Crowd_Bored--;
				}

				v = (a << 14) / 1000;

				if (v > 16384)
					v = 16384;

				DAT_800d10e8 ^= 1;

				if (v > 2048) {
					SFX_PlayX(SND_CROWDCHEER, v, 0);
					SFX_PlayX(SND_CROWDAPPLAUSE, v, 0);

					if (a >= 8000)
						SFX_PlayX(SND_CROWDAMAZED, v, 0);
				} else {
					if (a >= 400)
						SFX_PlayX(SND_CROWDAPPLAUSE, v * 4, 0);
				}
			}
		}
	}
}

/* Crowd_Oww__Fi (0x80010eb8) */
void Crowd_Oww(int Speed) {
	int Volume;

	if (Levels[GLevel].Crowd) {
		if (GGame == 1) {
			Volume = (Speed << 14) / 80;
			Crowd_Bored++;

			if (Volume > 16384)
				Volume = 16384;

			if (Volume < 2048)
				Volume = 2048;

			SFX_PlayX(SND_CROWDAMAZED, Volume, 0);
		}
	}
}

/* Crowd_Ahh__Fi (0x80010f70) */
void Crowd_Ahh(int a) {
	int s;

	if (Levels[GLevel].Crowd && GGame == 1) {
		if (a < 100)
			Crowd_Bored += 2;
		else
			Crowd_Bored = 0;

		s = (a << 14) / 1000;

		if (s > 16384)
			s = 16384;

		if (s < 4096)
			s = 4096;
		else
			SFX_PlayX(SND_CROWDAPPLAUSE, (short)(s / 2), 0);

		SFX_PlayX(SND_CROWDOW, (short)s, 0);
	}
}

/* FUN_80011060 (0x80011060) */
extern "C" void FUN_80011060(int a) {
	short s1, s0;

	if (Levels[GLevel].Crowd && GGame == 1) {
		s1 = (a * (Rnd(20) + 90)) / 100;
		s0 = (a * (Rnd(20) + 90)) / 100;
		SFX_PlayLR(SND_CROWDBOO, s1, s0, 0);
	}
}
