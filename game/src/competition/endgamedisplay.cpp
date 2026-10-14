/* Competition_EndGameDisplay__Fv (0x80076c4c) */
/* C++ (COMPETITION.CPP). Demo twin (0.98 by opcode); locals from its MAIN.SYM. The end-of-
   competition screen: either the "no medal" text with three bronze placeholders, or the results
   text and the medals won on the ten tracked events, spun in place, then the continue prompt once
   the first medal has settled. */
#include "game/competition.h"
#include "game/powerup.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/mess.h"

extern CBruce *GSkater;

/* GCareer.CharacterProgress[].Tapes viewed from its first Tapes byte (retail addresses it that way) */
struct SCareerRecord {
	uchar Flags[14];
};

extern SCareerRecord GCareer_c[];
extern char *GameText_180[];

void Mess_DeleteAll();
void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void Front_PrintCentredPrompt(const char *);
CPowerUp *Medal_Create(EMedalType, int, int, int, int, int);

void Competition_EndGameDisplay() {
	CCompetition *pComp = &theCompetition;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetRGB(122, 122, 122, 0);
	Mess_SetTextJustify(0);

	if (Competition_WonGameAllGold()) {
		Mess_DrawText(256, 40, "You scored all golds!", 0, 0, 256, 0);
		Mess_DrawText(256, 60, "Your are the top pro skater.", 0, 0, 256, 0);
		Mess_DrawText(256, 80, "Congratulations!", 0, 0, 256, 0);
		Mess_DrawText(256, 180, "Watch this, then get off your", 0, 0, 256, 0);
		Mess_DrawText(256, 200, "couch and go skate!", 0, 0, 256, 0);

		if (!pComp->mMedal1)
			pComp->mMedal1 = Medal_Create((EMedalType)5, -30, 5, 140, 1536, 3);

		if (!pComp->mMedal2)
			pComp->mMedal2 = Medal_Create((EMedalType)5, 0, 5, 140, 1536, 3);

		if (!pComp->mMedal3)
			pComp->mMedal3 = Medal_Create((EMedalType)5, 30, 5, 140, 1536, 3);
	} else {
		Mess_DrawText(256, 40, "Your third medal.. Good going!", 0, 0, 256, 0);
		Mess_DrawText(256, 60, "You just might become", 0, 0, 256, 0);
		Mess_DrawText(256, 80, "an average skater.", 0, 0, 256, 0);
		Mess_DrawText(256, 180, "pro skaters get all golds...", 0, 0, 256, 0);

		{
			EMedalType m[3];
			int n = 0;
			int i;

			for (i = 0; i < 10; i++) {
				short f = GCareer_c[GSkater->mSkaterIndex].Flags[i];

				if (f & 0x20)
					m[n++] = (EMedalType)5;
				else if (f & 0x40)
					m[n++] = (EMedalType)6;
				else if (f & 0x80)
					m[n++] = (EMedalType)7;
			}

			if (!pComp->mMedal1)
				pComp->mMedal1 = Medal_Create(m[0], -30, 5, 140, 1280, 5);

			if (!pComp->mMedal2)
				pComp->mMedal2 = Medal_Create(m[1], 0, 5, 140, 1280, 5);

			if (!pComp->mMedal3)
				pComp->mMedal3 = Medal_Create(m[2], 30, 5, 140, 1280, 5);
		}
	}

	pComp->mMedal1->AI();
	pComp->mMedal2->AI();
	pComp->mMedal3->AI();

	{
		int Done = 1;

		if (pComp->mMedal1) {
			int c = pComp->mMedal1->mTimer < 40;

			Done = c == 0;
		}

		if (Done && !pComp->mFading)
			Front_PrintCentredPrompt(GameText_180[0]);
	}
}
