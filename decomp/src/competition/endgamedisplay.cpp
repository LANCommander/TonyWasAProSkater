/* Competition_EndGameDisplay__Fv (0x80076c4c) */
/* C++ (COMPETITION.CPP). Demo twin (0.98 by opcode); locals from its MAIN.SYM. The end-of-
   competition screen: either the "no medal" text with three bronze placeholders, or the results
   text and the medals won on the ten tracked events, spun in place, then the continue prompt once
   the first medal has settled. */
#include "game/competition.h"
#include "game/powerup.h"
#include "game/bruce.h"
#include "game/career.h"

enum EMedalType { MEDAL_0 };

extern CBruce *GSkater;

/* GCareer.CharacterProgress[].Tapes viewed from its first Tapes byte (retail addresses it that way) */
struct SCareerRecord {
	uchar Flags[14];
};

extern SCareerRecord GCareer_c[];
extern char s_You_scored_all_golds[], s_Your_are_the_top_pro_skater[], s_Congratulations[], s_Watch_this_then_get_off_your[], s_couch_and_go_skate[];
extern char s_Your_third_medal_Good_going[], s_You_just_might_become[], s_an_average_skater[], s_pro_skaters_get_all_golds[];
extern char *GameText_180[];

void Mess_DeleteAll();
void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void Front_PrintCentredPrompt(const char *);
CPowerUp *Medal_Create(EMedalType, int, int, int, int, int);

extern "C" {
	void Mess_SetTextJustify__FUc(int);
	int Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
	int FUN_8007401c(void);
}

void Competition_EndGameDisplay() {
	CCompetition *pComp = &theCompetition;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetRGB(122, 122, 122, 0);
	Mess_SetTextJustify__FUc(0);

	if (FUN_8007401c()) {
		Mess_DrawText__FiiPCciUiib(256, 40, s_You_scored_all_golds, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 60, s_Your_are_the_top_pro_skater, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 80, s_Congratulations, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 180, s_Watch_this_then_get_off_your, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 200, s_couch_and_go_skate, 0, 0, 256, 0);

		if (!pComp->mMedal1)
			pComp->mMedal1 = Medal_Create((EMedalType)5, -30, 5, 140, 1536, 3);

		if (!pComp->mMedal2)
			pComp->mMedal2 = Medal_Create((EMedalType)5, 0, 5, 140, 1536, 3);

		if (!pComp->mMedal3)
			pComp->mMedal3 = Medal_Create((EMedalType)5, 30, 5, 140, 1536, 3);
	} else {
		Mess_DrawText__FiiPCciUiib(256, 40, s_Your_third_medal_Good_going, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 60, s_You_just_might_become, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 80, s_an_average_skater, 0, 0, 256, 0);
		Mess_DrawText__FiiPCciUiib(256, 180, s_pro_skaters_get_all_golds, 0, 0, 256, 0);

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
