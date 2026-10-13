/* Decompiled data: the retail executable's initialised globals that src/front/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"

extern int D_800d1d58[];

PSX_DATA(0x800a13c4) char *D_800a13c4[3] = {
	"T2_loose.bmp",
	"T2_mdium.bmp",
	"T2_tight.bmp",
};

PSX_DATA(0x800a14c0) char *D_800a14c0[15] = {
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

PSX_DATA(0x800a15cc) char *D_800a15cc[3] = {
	"T1_loose.bmp",
	"T1_mdium.bmp",
	"T1_tight.bmp",
};

PSX_DATA(0x800a16c8) char *D_800a16c8[15] = {
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

PSX_DATA(0x800a1884) char *D_800a1884[3] = {
	"Skate Park",
	"Burnside",
	"Roswell",
};

PSX_DATA(0x800a1d0c) char *DAT_800a1d0c[8] = {
	"TRIANGLE",
	"SQUARE",
	"CIRCLE",
	"X",
	"VIBRATION",
	"AUTOKICK",
	"RESET TO DEFAULT",
	"BACK TO OPTIONS",
};

PSX_DATA(0x800a2208) char *D_800a2208[3] = {
	"Skate Park",
	"Burnside",
	"Roswell",
};

/* THPS2 demo: CTape*[5] */
PSX_DATA(0x800a5b14) int GTape[5];

/* FallingTextEffect (src/front/update.cpp's GHorseFallingText; the code sets it up) */
PSX_DATA(0x800a5b28) int DAT_800a5b28[18];

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

PSX_DATA(0x800a5b70) SLevelFiles D_800a5b70[3] = {
	{"roof_lib", "roof_1", "roofback", 7374, -1181, 5332, 0, 1744, 768},
	{"sewr_lib", "sewr_4", "sewrback", -11280, 10930, 86269, 0, 3536, 2304},
	{"", "", ""},
};

/* THPS2 demo: int[8] */
PSX_DATA(0x800a5cec) int StatsLeft[8];

/* THPS2 demo: int[8] */
PSX_DATA(0x800a5d0c) int StatsRight[8];

/* THPS2 demo: char[24] */
PSX_DATA(0x800a5d2c) unsigned char soundBuf[24];

/* THPS2 demo: char[24] */
PSX_DATA(0x800a5d44) unsigned char musicBuf[24];

PSX_DATA(0x800a8240) unsigned char GBoardStats[5];

PSX_DATA(0x800be42c) int DAT_800be42c[3] = {
	5, 3, 1,
};

PSX_DATA(0x800be438) int DAT_800be438[3] = {
	5, 3, 1,
};

/* THPS2 demo: char[30] */
PSX_DATA(0x800bf0c4) unsigned char DemoName[16];

PSX_DATA(0x800bf0d4) int DAT_800bf0d4[2] = {
	40, 39,
};

PSX_DATA(0x800bf0e0) int DAT_800bf0e0[2] = {
	184, 172,
};

PSX_DATA(0x800bf260) int DAT_800bf260[8] = {
	0, 1, 2, 3, 0, 1, 2, 3,
};

PSX_DATA(0x800bf280) int DAT_800bf280[8];
PSX_DATA(0x800bf2a0) int DAT_800bf2a0[10];
PSX_DATA(0x800bf2c8) int DAT_800bf2c8[10];

/* THPS2 demo: int */
PSX_DATA(0x800d1100) int SizeUp[1];

/* THPS2 demo: int */
PSX_DATA(0x800d13c4) int GHorseContinue[1];

/* THPS2 demo: null */
PSX_DATA(0x800d13d8) int GRollCredits[1];

/* THPS2 demo: int */
PSX_DATA(0x800d13dc) int GHighScoreLineIndex[1] = {
	-1,
};

/* THPS2 demo: int */
PSX_DATA(0x800d13e0) int GHighScoreInitialCursorPos[1];

/* THPS2 demo: int */
PSX_DATA(0x800d13e4) int GSoftResetEnabled[1] = {
	1,
};

/* EPausedStatus (THPS2 demo: EPausedStatus) */
PSX_DATA(0x800d13e8) int Status[1];

PSX_DATA(0x800d13ec) int DAT_800d13ec[2] = {
	48,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1410) int WobbleAmplitude[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1414) int WobbleT[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1418) int UpDownAutoRepeatTimer[1];

/* THPS2 demo: SAnimFrame* */
PSX_DATA(0x800d1420) int ButtonsAnim[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1424) int VoiceLevelGotFocus[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d142c) int pESMenu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1430) int pVSMenu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1434) int pVS2Menu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1438) int pVS3Menu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d143c) int pEHMenu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1440) int pSoundMenu[1];

PSX_DATA(0x800d1444) int DAT_800d1444[12] = {
	0, 0x1ff0200, 0, 0, 0, 0xc8006a, 0xc012c, 0xc9006c, 0xa0128, 0, 0xc9006c, 0xa0000,
};

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1474) int pPausedMenu[1];

/* THPS2 demo: CMenu* */
PSX_DATA(0x800d1478) int pRestartsMenu[1];

PSX_DATA(0x800d14d8) int DAT_800d14d8[1];
PSX_DATA(0x800d14dc) int DAT_800d14dc[1];

/* THPS2 demo: int */
PSX_DATA(0x800d14f4) int GHorseSounded[1];

/* THPS2 demo: int */
PSX_DATA(0x800d14f8) int GHorseScale[1];

/* THPS2 demo: int */
PSX_DATA(0x800d14fc) int NextHorseMode[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1500) int NextHorsePlayer[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1504) int NextHorseScore[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1508) int NextHorseRestart[1];

/* THPS2 demo: int[2] */
PSX_DATA(0x800d150c) int NextHorseLetters[2];

/* THPS2 demo: int */
PSX_DATA(0x800d1514) int Front_OnTime[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1518) int StatsMode[1];

/* THPS2 demo: int */
PSX_DATA(0x800d151c) int GStart1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1520) int GStart2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16a8) int GAutoTest[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16ac) int GAutoPlayer1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16b0) int GAutoPlayer2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16b4) int GAutoLevel[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16b8) int GAutoGame[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16cc) int GHorseWordChosen[1];

/* THPS2 demo: int */
PSX_DATA(0x800d16d8) int SelectedSkater2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1708) int Cheat_LevelSelect[1];

/* THPS2 demo: int */
PSX_DATA(0x800d170c) int Cheat_AutoTest[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1710) int Cheat_RestartSkip[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1714) int Cheat_GameInfo[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1724) int Cheat_BigHead[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1764) int CurrentGameLevel[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1784) int GState[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17a0) int GHorseMode[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17a4) int GHorseScore[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17a8) int GHorseDoneStart[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17ac) int GHorseRestart[1];

/* THPS2 demo: int[2] */
PSX_DATA(0x800d17b0) int GHorseLetters[2];

/* THPS2 demo: int */
PSX_DATA(0x800d17b8) int GHorseCursorPos[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17bc) int GHorseWaitTime[1];

/* THPS2 demo: int */
PSX_DATA(0x800d17c0) int GHorseClearInput2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1954) int GWins1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1958) int GLosses1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d195c) int GStreak1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1960) int GHighTags1[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1964) int GWins2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1968) int GLosses2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d196c) int GStreak2[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1970) int GHighTags2[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d1a1c) unsigned int GameFade[1];

PSX_DATA(0x800d1bac) int D_800d1bac[2] = {
	0, 368,
};

PSX_DATA(0x800d1bb4) int DAT_800d1bb4[2] = {
	368, 272,
};

PSX_DATA(0x800d1bbc) int DAT_800d1bbc[2] = {
	92, 374,
};

PSX_DATA(0x800d1bc4) int DAT_800d1bc4[2] = {
	188, 460,
};

PSX_DATA(0x800d1bcc) int DAT_800d1bcc[2] = {
	135, 423,
};

PSX_DATA(0x800d1bd4) int DAT_800d1bd4[2] = {
	172, 460,
};

PSX_DATA(0x800d1bdc) int DAT_800d1bdc[2] = {
	199, 487,
};

PSX_DATA(0x800d1be4) int DAT_800d1be4[2] = {
	236, 524,
};

PSX_DATA(0x800d1bec) int DAT_800d1bec[2] = {
	265, 553,
};

PSX_DATA(0x800d1bf4) int DAT_800d1bf4[2] = {
	294, 582,
};

PSX_DATA(0x800d1c1c) int D_800d1c1c[1] = {
	32,
};

PSX_DATA(0x800d1c20) int DAT_800d1c20[1];
PSX_DATA(0x800d1c24) int DAT_800d1c24[1];
PSX_DATA(0x800d1c40) int DAT_800d1c40[1];
PSX_DATA(0x800d1c44) int DAT_800d1c44[1];

PSX_DATA(0x800d1c48) int DAT_800d1c48[1] = {
	0x11b1,
};

PSX_DATA(0x800d1c4c) int DAT_800d1c4c[1] = {
	0x11b1,
};

PSX_DATA(0x800d1c50) int DAT_800d1c50[1] = {
	30,
};

PSX_DATA(0x800d1c54) int DAT_800d1c54[1] = {
	70,
};

PSX_DATA(0x800d1c58) int DAT_800d1c58[1] = {
	16,
};

PSX_DATA(0x800d1c5c) int DAT_800d1c5c[1] = {
	-1,
};

PSX_DATA(0x800d1c60) int DAT_800d1c60[1];
PSX_DATA(0x800d1c64) int DAT_800d1c64[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1c9c) int StartScreen[1];

PSX_DATA(0x800d1cb4) int DAT_800d1cb4[1];
PSX_DATA(0x800d1cb8) int DAT_800d1cb8[1];
PSX_DATA(0x800d1d00) int DAT_800d1d00[1];
PSX_DATA(0x800d1d04) int DAT_800d1d04[1];
PSX_DATA(0x800d1d08) int DAT_800d1d08[1];

PSX_DATA(0x800d1d58) int D_800d1d58[1] = {
	88,
};

PSX_DATA(0x800d1dc0) int DAT_800d1dc0[1];
PSX_DATA(0x800d1dc4) int DAT_800d1dc4[1];

PSX_DATA(0x800d1dc8) int DAT_800d1dc8[1] = {
	-1,
};

PSX_DATA(0x800d1dd4) int D_800d1dd4[1] = {
	48,
};

/* THPS2 demo: char* */
PSX_DATA(0x800d1f84) char *TrigFileName[2] = {
	"prsn_t",
	"********blaablaa",
};

/* THPS2 demo: int */
PSX_DATA(0x800d2130) int GHeadUp[1] = {
	7,
};
