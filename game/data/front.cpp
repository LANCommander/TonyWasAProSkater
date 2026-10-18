/* Decompiled data: the retail executable's initialised globals that src/front/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/career.h"
#include "game/tapes.h"

struct CMenu;
struct Sprite;

extern int D_800d1d58[];

char *D_800a13c4[3] = {
	"T2_loose.bmp",
	"T2_mdium.bmp",
	"T2_tight.bmp",
};

char *D_800a14c0[15] = {
	"W2blue01.bmp",
	"W2blue02.bmp",
	"W2blue03.bmp",
	"W2blue04.bmp",
	"W2brow01.bmp",
	"W2dark01.bmp",
	"W2gree01.bmp",
	"W2gree02.bmp",
	"W2gree03.bmp",
	"W2oran01.bmp",
	"W2purp01.bmp",
	"W2red_01.bmp",
	"W2red_02.bmp",
	"W2whit01.bmp",
	"W2yell01.bmp",
};

char *D_800a15cc[3] = {
	"T1_loose.bmp",
	"T1_mdium.bmp",
	"T1_tight.bmp",
};

char *D_800a16c8[15] = {
	"W1blue01.bmp",
	"W1blue02.bmp",
	"W1blue03.bmp",
	"W1blue04.bmp",
	"W1brow01.bmp",
	"W1dark01.bmp",
	"W1gree01.bmp",
	"W1gree02.bmp",
	"W1gree03.bmp",
	"W1oran01.bmp",
	"W1purp01.bmp",
	"W1red_01.bmp",
	"W1red_02.bmp",
	"W1whit01.bmp",
	"W1yell01.bmp",
};

char *D_800a1884[3] = {
	"Skate Park",
	"Burnside",
	"Roswell",
};

char *DAT_800a1d0c[8] = {
	"TRIANGLE",
	"SQUARE",
	"CIRCLE",
	"X",
	"VIBRATION",
	"AUTOKICK",
	"RESET TO DEFAULT",
	"BACK TO OPTIONS",
};

char *D_800a2208[3] = {
	"Skate Park",
	"Burnside",
	"Roswell",
};

/* THPS2 demo: CTape*[5] */
CTape *GTape[5];

/* FallingTextEffect (Front_Update's GHorseFallingText, src/front.cpp; the code sets it up) */
int DAT_800a5b28[18];

/* three level entries the decompiled code doesn't refer to: library, level and background
 * files, a position, then two sizes */
struct SLevelFiles {
	char *pLib;
	char *pLevel;
	char *pBack;
	int x, y, z;
	int w6;
	int Width, Height;
};

SLevelFiles D_800a5b70[3] = {
	{"roof_lib", "roof_1", "roofback", 7374, -1181, 5332, 0, 1744, 768},
	{"sewr_lib", "sewr_4", "sewrback", -11280, 10930, 86269, 0, 3536, 2304},
	{"", "", ""},
};

/* THPS2 demo: int[8] */
int StatsLeft[8];

/* THPS2 demo: int[8] */
int StatsRight[8];

/* THPS2 demo: char[24] */
unsigned char soundBuf[24];

/* THPS2 demo: char[24] */
unsigned char musicBuf[24];

unsigned char GBoardStats[5];

int DAT_800be42c[3] = {
	5, 3, 1,
};

int DAT_800be438[3] = {
	5, 3, 1,
};

/* THPS2 demo: char[30] */
unsigned char DemoName[16];

int DAT_800bf0d4[3] = {
	40, 39, 44,
};

int DAT_800bf0e0[3] = {
	184, 172, 184,
};

int DAT_800bf260[8] = {
	0, 1, 2, 3, 0, 1, 2, 3,
};

int DAT_800bf280[8];
int DAT_800bf2a0[MAX_LEVELS];
int DAT_800bf2c8[MAX_LEVELS];

/* THPS2 demo: int */
int SizeUp[1];

/* THPS2 demo: int */
int GHorseContinue[1];

/* THPS2 demo: null */
int GRollCredits[1];

/* THPS2 demo: int */
int GHighScoreLineIndex[1] = {
	-1,
};

/* THPS2 demo: int */
int GHighScoreInitialCursorPos[1];

/* THPS2 demo: int */
int GSoftResetEnabled[1] = {
	1,
};

/* EPausedStatus (THPS2 demo: EPausedStatus) */
int Status[1];

int DAT_800d13ec[2] = {
	48,
};

/* THPS2 demo: int */
int WobbleAmplitude[1];

/* THPS2 demo: int */
int WobbleT[1];

/* THPS2 demo: int */
int UpDownAutoRepeatTimer[1];

/* THPS2 demo: SAnimFrame* */
void *ButtonsAnim;

/* THPS2 demo: int */
int VoiceLevelGotFocus[1];

/* THPS2 demo: CMenu* */
CMenu *pESMenu;

/* THPS2 demo: CMenu* */
CMenu *pVSMenu;

/* THPS2 demo: CMenu* */
CMenu *pVS2Menu;

/* THPS2 demo: CMenu* */
CMenu *pVS3Menu;

/* THPS2 demo: CMenu* */
CMenu *pEHMenu;

/* THPS2 demo: CMenu* */
CMenu *pSoundMenu;

int DAT_800d1444[12] = {
	0, 0x1ff0200, 0, 0, 0, 0xc8006a, 0xc012c, 0xc9006c, 0xa0128, 0, 0xc9006c, 0xa0000,
};

/* THPS2 demo: CMenu* */
CMenu *pPausedMenu;

/* THPS2 demo: CMenu* */
CMenu *pRestartsMenu;

int DAT_800d14d8[1];
int DAT_800d14dc[1];

/* THPS2 demo: int */
int GHorseSounded[1];

/* THPS2 demo: int */
int GHorseScale[1];

/* THPS2 demo: int */
int NextHorseMode[1];

/* THPS2 demo: int */
int NextHorsePlayer[1];

/* THPS2 demo: int */
int NextHorseScore[1];

/* THPS2 demo: int */
int NextHorseRestart[1];

/* THPS2 demo: int[2] */
int NextHorseLetters[2];

/* THPS2 demo: int */
int Front_OnTime[1];

/* THPS2 demo: int */
int StatsMode[1];

/* THPS2 demo: int */
int GStart1[1];

/* THPS2 demo: int */
int GStart2[1];

/* THPS2 demo: int */
int GAutoTest[1];

/* THPS2 demo: int */
int GAutoPlayer1[1];

/* THPS2 demo: int */
int GAutoPlayer2[1];

/* THPS2 demo: int */
int GAutoLevel[1];

/* THPS2 demo: int */
int GAutoGame[1];

/* THPS2 demo: int */
int GHorseWordChosen[1];

/* THPS2 demo: int */
int SelectedSkater2[1];

/* THPS2 demo: int */
int Cheat_LevelSelect[1];

/* THPS2 demo: int */
int Cheat_AutoTest[1];

/* THPS2 demo: int */
int Cheat_RestartSkip[1];

/* THPS2 demo: int */
int Cheat_GameInfo[1];

/* THPS2 demo: int */
int Cheat_BigHead[1];

/* THPS2 demo: int */
int CurrentGameLevel[1];

/* THPS2 demo: int */
int GState[1];

/* THPS2 demo: int */
int GHorseMode[1];

/* THPS2 demo: int */
int GHorseScore[1];

/* THPS2 demo: int */
int GHorseDoneStart[1];

/* THPS2 demo: int */
int GHorseRestart[1];

/* THPS2 demo: int[2] */
int GHorseLetters[2];

/* THPS2 demo: int */
int GHorseCursorPos[1];

/* THPS2 demo: int */
int GHorseWaitTime[1];

/* THPS2 demo: int */
int GHorseClearInput2[1];

/* THPS2 demo: int */
int GWins1[1];

/* THPS2 demo: int */
int GLosses1[1];

/* THPS2 demo: int */
int GStreak1[1];

/* THPS2 demo: int */
int GHighTags1[1];

/* THPS2 demo: int */
int GWins2[1];

/* THPS2 demo: int */
int GLosses2[1];

/* THPS2 demo: int */
int GStreak2[1];

/* THPS2 demo: int */
int GHighTags2[1];

/* THPS2 demo: uint */
unsigned int GameFade[1];

int DAT_800d1bac[2] = {
	0, 368,
};

int DAT_800d1bb4[2] = {
	368, 272,
};

int DAT_800d1bbc[2] = {
	92, 374,
};

int DAT_800d1bc4[2] = {
	188, 460,
};

int DAT_800d1bcc[2] = {
	135, 423,
};

int DAT_800d1bd4[2] = {
	172, 460,
};

int DAT_800d1bdc[2] = {
	199, 487,
};

int DAT_800d1be4[2] = {
	236, 524,
};

int DAT_800d1bec[2] = {
	265, 553,
};

int DAT_800d1bf4[2] = {
	294, 582,
};

int DAT_800d1c1c[1] = {
	32,
};

int DAT_800d1c20[1];
int DAT_800d1c24[1];
int DAT_800d1c40[1];
int DAT_800d1c44[1];

int DAT_800d1c48[1] = {
	0x11b1,
};

int DAT_800d1c4c[1] = {
	0x11b1,
};

int DAT_800d1c50[1] = {
	30,
};

int DAT_800d1c54[1] = {
	70,
};

int DAT_800d1c58[1] = {
	16,
};

int DAT_800d1c5c[1] = {
	-1,
};

int DAT_800d1c60[1];
int DAT_800d1c64[1];

/* THPS2 demo: int */
int StartScreen[1];

int DAT_800d1cb4[1];
int DAT_800d1cb8[1];
Sprite *DAT_800d1d00;
Sprite *DAT_800d1d04;
int DAT_800d1d08[1];

int D_800d1d58[1] = {
	88,
};

int DAT_800d1dc0[1];
int DAT_800d1dc4[1];

int DAT_800d1dc8[1] = {
	-1,
};

int D_800d1dd4[1] = {
	48,
};

/* THPS2 demo: char* */
char *TrigFileName[2] = {
	"prsn_t",
	"********blaablaa",
};

/* THPS2 demo: int */
int GHeadUp[1] = {
	7,
};
