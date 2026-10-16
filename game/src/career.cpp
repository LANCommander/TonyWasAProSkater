/* CAREER.CPP: the career save (GCareer) and the options: their defaults, copying the
   preferences between the save and the live settings, the controller mapping, and the goals
   (tapes) and medals a skater has won. Declarations in game/career.h. */
#include "game/career.h"
#include "game/psyq.h"
#include "game/pad.h"
#include "game/bruce.h"
#include "game/globals.h"
#include "game/utils.h"
#include "pc_mods.h"

extern char *DefaultHighScoreInitials[];
/* The tables of levels 10-19 (THPS1 has 10): retail took their base score from past the end of
   Levels, i.e. from whatever data follows it (the card directory among it); the port uses what
   retail read there on a cold boot, so saves stay the same whatever the memory layout. */
static const int UnusedLevelHiScore[10] = {3, 0, 0, 0, 0, 0, 0x23002c, 0, 1, 0};
void Options_RemapControllers();

/* Career_Init__Fv (0x80010000) */
/* Demo twin; locals from its MAIN.SYM. Clears the career save, stamps its
   version, then fills every level's high-score table with random names from the name list,
   scores stepping down 5000 from the level's base and random skaters. THPS1 has 20 levels and
   no Career_Test call. */
void Career_Init() {
	char *p;
	char **pHighScore;
	int HighScoreCount;

	{
		int i;

		p = (char *)&GCareer;

		for (i = 0; i < sizeof(SCareer); i++)
			*p++ = 0;
	}

	GCareer.Version = 0x60299;
	pHighScore = DefaultHighScoreInitials;
	HighScoreCount = 0;

	while (*pHighScore) {
		pHighScore++;
		HighScoreCount++;
	}

	{
		int Level;

		for (Level = 0; Level < 20; Level++) {
			int Score;

			for (Score = 0; Score < 8; Score++) {
				pHighScore = &DefaultHighScoreInitials[Rnd(HighScoreCount)];
				GCareer.HighScore[Level][Score].Name[0] = (*pHighScore)[0];
				GCareer.HighScore[Level][Score].Name[1] = (*pHighScore)[1];
				GCareer.HighScore[Level][Score].Name[2] = (*pHighScore)[2];
				GCareer.HighScore[Level][Score].Score =
				    (Level < STOCK_LEVELS ? Levels[Level].HiScore
				     : Level < 10         ? 0 /* the end entry's (mods' levels: Career_InitExt) */
				                          : UnusedLevelHiScore[Level - 10]) +
				    (7 - Score) * 5000;
				GCareer.HighScore[Level][Score].Skater = Rnd(10);
			}
		}
	}

	Career_InitExt();
	GHasCareerLoaded = 0;
	GLoadedCareerIsSaved = 0;

	{
		int i;

		for (i = 0; i < 2; i++)
			GCareer.LastTrucks[i] = 1;
	}

	Options_SetPreferences();
}

/* Options_SetPreferences__Fv (0x800101e4) */
/* Demo twin; locals from its MAIN.SYM. Copies the default preferences into the
   career save's options (volumes, per-pad auto-kick/vibration/button mapping, display toggles,
   the HORSE word and the last high-score initials), sets the volume scalers, then remaps pads. */
void Options_SetPreferences() {
	GCareer.Options.SFXVolume = GPreferences.SFXVolume;
	GCareer.Options.MusicVolume = GPreferences.MusicVolume;
	GCareer.Options.TrickTipsEnabled = GPreferences.TrickTipsEnabled;
	GCareer.Options.ScoreDisplayEnabled = GPreferences.ScoreDisplayEnabled;

	{
		int padNum;

		for (padNum = 0; padNum < 2; padNum++) {
			GCareer.Options.AutoKickEnabled[padNum] = GPreferences.AutoKickEnabled[padNum];
			GCareer.Options.VibrationEnabled[padNum] = GPreferences.VibrationEnabled[padNum];
			GCareer.Options.ControllerFunction[padNum][0] =
			    GPreferences.ControllerFunction[padNum][0];
			GCareer.Options.ControllerFunction[padNum][1] =
			    GPreferences.ControllerFunction[padNum][1];
			GCareer.Options.ControllerFunction[padNum][2] =
			    GPreferences.ControllerFunction[padNum][2];
			GCareer.Options.ControllerFunction[padNum][3] =
			    GPreferences.ControllerFunction[padNum][3];
		}
	}

	{
		int i;

		SFXLEVEL = GCareer.Options.SFXVolume * 1365;
		XALEVEL = GCareer.Options.MusicVolume * 1365;
		sprintf(GCareer.Options.HorseWord, GPreferences.HorseWord);

		GCareer.Options.HorseLen = GPreferences.HorseLen;

		for (i = GPreferences.HorseLen; i < 11; i++)
			GCareer.Options.HorseWord[i] = 0;

		sprintf(GCareer.Options.LastHighScoreInitials, GPreferences.LastHighScoreInitials);
	}

	Options_RemapControllers();
}

/* Options_GetPreferences__Fv (0x800103a4) */
/* The reverse of Options_SetPreferences: copies the saved preferences (GCareer.Options) into the
   live game settings (GPreferences), the horse word zero-padded to 11 bytes. */
void Options_GetPreferences() {
	int i;

	GPreferences.SFXVolume = GCareer.Options.SFXVolume;
	GPreferences.MusicVolume = GCareer.Options.MusicVolume;
	GPreferences.TrickTipsEnabled = GCareer.Options.TrickTipsEnabled;
	GPreferences.ScoreDisplayEnabled = GCareer.Options.ScoreDisplayEnabled;

	for (i = 0; i < 2; i++) {
		GPreferences.AutoKickEnabled[i] = GCareer.Options.AutoKickEnabled[i];
		GPreferences.VibrationEnabled[i] = GCareer.Options.VibrationEnabled[i];
		GPreferences.ControllerFunction[i][0] = GCareer.Options.ControllerFunction[i][0];
		GPreferences.ControllerFunction[i][1] = GCareer.Options.ControllerFunction[i][1];
		GPreferences.ControllerFunction[i][2] = GCareer.Options.ControllerFunction[i][2];
		GPreferences.ControllerFunction[i][3] = GCareer.Options.ControllerFunction[i][3];
	}

	sprintf(GPreferences.HorseWord, GCareer.Options.HorseWord);

	GPreferences.HorseLen = GCareer.Options.HorseLen;

	{
		int j;

		for (j = GPreferences.HorseLen; j < 11; j++)
			GPreferences.HorseWord[j] = 0;
	}

	sprintf(GPreferences.LastHighScoreInitials, GCareer.Options.LastHighScoreInitials);
}

/* Options_RemapControllers__Fv (0x80010518) */
/* Demo twin. Points each pad's pTriangle/pSquare/pCircle/pX at the button the career options map it to. */
void Options_RemapControllers() {
}

/* Options_Init__Fv (0x800105f4) */
/* The default options: volumes, trick tips and score display on, auto-kick and vibration on,
   the controllers' default button mapping, "HORSE" and the high-score initials "AAA". */
void Options_Init() {
	int i;

	GPreferences.SFXVolume = 12;
	SFXLEVEL = 16380;
	GPreferences.MusicVolume = 6;
	XALEVEL = 8190;
	GPreferences.TrickTipsEnabled = 1;
	GPreferences.ScoreDisplayEnabled = 1;

	for (i = 0; i < 2; i++) {
		GPreferences.AutoKickEnabled[i] = 1;
		GPreferences.VibrationEnabled[i] = 1;
		GPreferences.ControllerFunction[i][0] = 0;
		GPreferences.ControllerFunction[i][1] = 1;
		GPreferences.ControllerFunction[i][2] = 2;
		GPreferences.ControllerFunction[i][3] = 3;
	}

	sprintf(GPreferences.HorseWord, "HORSE");

	GPreferences.HorseLen = 5;
	sprintf(GPreferences.LastHighScoreInitials, "AAA");
}

/* Career_Got__Fi (0x800106e0) */
/* C++ (CAREER.CPP), next to Career_GiveTape. Whether the current skater has goal (tape) `Goal` of
   the current level. */
int Career_Got(int Goal) {
	return (*Career_Tapes(GSkater[0]->mSkaterIndex, GLevel) >> Goal) & 1;
}

/* Career_GiveTape__Fii (0x80010724) */
/* Demo twin; locals from its MAIN.SYM. Marks a level goal (tape) as got for the
   current skater, recounts the skater's goals over the ten levels that count, and bumps the
   skater's goal-got counter. */
void Career_GiveTape(int Level, int Goal) {
	int t = 0;

	*Career_Tapes(GSkater[0]->mSkaterIndex, Level) |= 1 << Goal;

	{
		int j;

		/* (the game's own levels: the career save counts theirs) */
		for (j = 0; j < STOCK_LEVELS; j++) {
			int i;

			for (i = 0; i < 5; i++) {
				if (!Levels[j].isCompetition &&
				    ((*Career_Tapes(GSkater[0]->mSkaterIndex, j) >> i) & 1))
					t++;
			}
		}
	}

	Career_Progress(GSkater[0]->mSkaterIndex)->NumTapes = t;
	GSkater[0]->mNumTapes++;
}

/* Career_GiveMedal__Fi10EMedalType (0x8001081c) */
/* Demo twin; locals from its MAIN.SYM. Records a competition medal for the
   current skater on that level (bits 5-7 of the level byte: gold, silver, bronze) and recounts
   the skater's medals over the ten levels that count. */
void Career_GiveMedal(int Level, EMedalType medal) {
	int g = 0;
	int s = 0;
	int b = 0;

	*Career_Tapes(GSkater[0]->mSkaterIndex, Level) |= 1 << medal;

	{
		int i;

		for (i = 0; i < STOCK_LEVELS; i++) {
			if (!Levels[i].isCompetition) {
				uchar t = *Career_Tapes(GSkater[0]->mSkaterIndex, i);

				if (t & 0x20)
					g++;

				if (t & 0x40)
					s++;

				if (t & 0x80)
					b++;
			}
		}
	}

	Career_Progress(GSkater[0]->mSkaterIndex)->NumGold = g;
	Career_Progress(GSkater[0]->mSkaterIndex)->NumSilver = s;
	Career_Progress(GSkater[0]->mSkaterIndex)->NumBronze = b;
}

/* Career_LevelTapes__Fi (0x80010944) */
/* Demo twin; locals from its MAIN.SYM. Counts the current skater's tapes (goals)
   on the current level; THPS1 ignores the Level argument. */
int Career_LevelTapes(int Level) {
	int t = 0;

	{
		int i;

		for (i = 0; i < 5; i++)
			if ((*Career_Tapes(GSkater[0]->mSkaterIndex, GLevel) >> i) & 1)
				t++;
	}

	return t;
}

/* ---- PC: the mods' skaters and levels (game/career.h) ---- */

/* where a skater index or level number out of the tables' range reads and writes (a save made
   with mods that are gone) */
static SSkaterCareer NoProgress;
static uchar NoTapes;
static SHighScore NoHighScores[8];
static int NoMedalMessages;

SSkaterCareer *Career_Progress(int skater) {
	if ((unsigned)skater < STOCK_SKATERS)
		return &GCareer.CharacterProgress[skater];
	if ((unsigned)skater < MAX_SKATERS)
		return &GCareerExt.CharacterProgress[skater - STOCK_SKATERS];
	return &NoProgress;
}

uchar *Career_Tapes(int skater, int level) {
	if ((unsigned)level < STOCK_LEVELS)
		return &Career_Progress(skater)->Tapes[level];
	if ((unsigned)skater < MAX_SKATERS && (unsigned)level < MAX_LEVELS - 1)
		return &GCareerExt.Tapes[skater][level - STOCK_LEVELS];
	return &NoTapes;
}

SHighScore *Career_HighScores(int level) {
	if ((unsigned)level < STOCK_LEVELS)
		return GCareer.HighScore[level];
	if ((unsigned)level < MAX_LEVELS - 1)
		return GCareerExt.HighScore[level - STOCK_LEVELS];
	return NoHighScores;
}

int *Career_MedalMessages(int skater) {
	if ((unsigned)skater < STOCK_SKATERS)
		return &GCareer.Options.MedalMessagesUsed[skater];
	if ((unsigned)skater < MAX_SKATERS)
		return &GCareerExt.MedalMessagesUsed[skater - STOCK_SKATERS];
	return &NoMedalMessages;
}

int Level_InMode(int level, int game) {
	int mode;

	if ((unsigned)level >= MAX_LEVELS)
		return 0;

	switch (game) {
		case 1:
			mode = LEVEL_CAREER;
			break;
		case 2:
			mode = LEVEL_FREESKATE;
			break;
		case 3:
			mode = LEVEL_SINGLESESSION;
			break;
		default:
			mode = LEVEL_MULTIPLAYER;
			break;
	}

	return (LevelModes[level] & mode) != 0;
}

/* The mods' part of a new career: no progress, and high-score tables made as Career_Init makes
   the game's (only for levels mods add, so a game without them draws no extra random numbers). */
void Career_InitExt() {
	int count = 0;
	char **pHighScore;
	char *p = (char *)&GCareerExt;

	for (int i = 0; i < (int)sizeof(SCareerExt); i++)
		*p++ = 0;

	for (pHighScore = DefaultHighScoreInitials; *pHighScore; pHighScore++)
		count++;

	for (int level = STOCK_LEVELS; level < MAX_LEVELS - 1 && *Levels[level].pLevelName; level++) {
		SHighScore *h = Career_HighScores(level);

		for (int rank = 0; rank < 8; rank++) {
			pHighScore = &DefaultHighScoreInitials[Rnd(count)];
			h[rank].Name[0] = (*pHighScore)[0];
			h[rank].Name[1] = (*pHighScore)[1];
			h[rank].Name[2] = (*pHighScore)[2];
			h[rank].Score = Levels[level].HiScore + (7 - rank) * 5000;
			h[rank].Skater = Rnd(10);
		}
	}
}

void Career_LoadExt(const char *name) {
	Career_InitExt();
	PC_ModCareerLoad(name);
}
