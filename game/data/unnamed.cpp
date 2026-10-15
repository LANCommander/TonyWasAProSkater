/* Decompiled data: the retail executable's initialised globals that src/unnamed/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/m3d.h"
#include "game/mem.h"
#include "game/trickob.h"
#include "game/pad.h"
#include "game/video.h"

struct CBody;
struct CBruce;
struct CMenu;
struct CRail;
struct PREManager;
struct SBlockHeader;
struct SDoubleBuffer;
struct SPalette;
struct Sprite;
#include "game/career.h"
#include "game/front.h"
#include "game/gamefmv.h"
#include "game/panel.h"
#include "game/shellutils.h"
#include "game/rail.h"

extern char dtor_8006553c[] PSX_NAME("FUN_8006553c");
extern char Delay[];
extern char FUN_8001d21c[];
extern char FUN_8001f274[];
extern char FUN_8001f2dc[];
extern char FUN_8004f544[];
extern char FUN_8005de48[];
extern char FUN_80066180[];
extern char FUN_80071a14[];
extern char FUN_80071acc[];
extern char FUN_80072e60[];
extern char FUN_80072e94[];
extern char FUN_80096f18[];

extern int D_8009f440[];
extern int D_800d15c4[];
extern int D_800d15d0[];
extern int D_800d15d4[];
extern int D_800d15d8[];
extern int D_800d15dc[];

int DAT_8009e31c[34] = {
	0, 0x6c756b73, 0x745f, 0x5f655200, 0x72617453, 0x63535f74, 0x6c6f6f68, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0x10000, 0, 0, 0, 0, 0, 0, 0, 0, 100, 3, 5, 1, 0x2000, 48, 180, 0x2080401, 0x2080401,
};

PsxVtEntry DAT_8009e548[2] = {
	{0, 0, 0},
	{0, 0, FUN_8001d21c},
};

PsxVtEntry DAT_8009ef94[2] = {
	{0, 0, 0},
	{0, 0, FUN_8001f274},
};

PsxVtEntry DAT_8009efa4[2] = {
	{0, 0, 0},
	{0, 0, FUN_8001f2dc},
};

int D_8009f440[4] = {
	0x6c623157, 0x31306575, 0x706d622e,
};

char *DAT_8009f530[15] = {
	(char *)D_8009f440,
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

unsigned char DAT_800a0000[1] = {
	0x18,
};

unsigned char DAT_800a0001[1] = {
	0x84,
};

unsigned char DAT_800a0002[1] = {
	0x03,
};

unsigned char DAT_800a0003[1] = {
	0x80,
};

unsigned char DAT_800a0004[1] = {
	0xd0,
};

unsigned char DAT_800a0005[3] = {
	0x84, 0x03, 0x80,
};

int DAT_800a0050[2] = {
	0x80038430,
};

int DAT_800a00d0[2] = {
	0x8003aa48,
};

short D_800a0820[3] = {
	0x525b, 0x4f4f, 0x2046,
};

unsigned char DAT_800a0826[1] = {
	0x32,
};

unsigned char DAT_800a0827[13] = {
	0x20, 0x52, 0x4f, 0x4f, 0x46, 0x20, 0x47, 0x41, 0x50, 0x5d,
};

int D_800a097c[2] = {
	0x494f475b, 0x4420474e,
};

int DAT_800a0984[3] = {
	0x204e574f, 0x5d504147,
};

PsxVtEntry DAT_800a1330[2] = {
	{0, 0, 0},
	{0, 0, FUN_8004f544},
};

PsxVtEntry DAT_800a1b94[2] = {
	{0, 0, 0},
	{0, 0, FUN_8005de48},
};

char *DAT_800a1c44[6] = {
	"CAREER MODE",
	"2 PLAYERS",
	"SINGLE SESSION",
	"FREE SKATE",
	"OPTIONS",
	"VIEW VIDEOS",
};

char *DAT_800a1c84[3] = {
	"CONTINUE GAME",
	"NEW GAME",
	"LOAD GAME",
};

char *DAT_800a1cac[3] = {
	"GRAFFITI",
	"TRICK ATTACK",
	"HORSE",
};

char *DAT_800a1d7c[3] = {
	"SFX LEVEL",
	"MUSIC LEVEL",
	"BACK TO OPTIONS",
};

char *DAT_800a1df8[7] = {
	"PLAYER 1 CONTROLS",
	"PLAYER 2 CONTROLS",
	"SOUND LEVELS",
	"TRICK TIPS",
	"SCORE DISPLAY",
	"LOAD DATA",
	"BACK TO MAIN",
};

char *DAT_800a1e4c[5] = {
	"LOAD REPLAY",
	"CINEMATICS",
	"411VM",
	"MUSIC CREDITS",
	"GAME CREDITS",
};

char *DAT_800a1eb4[7] = {
	"INTRO",
	"TONY HAWK",
	"BOB BURNQUIST",
	"GEOFF ROWLEY",
	"BUCKY LASEK",
	"CHAD MUSKA",
	"MORE CINEMATICS",
};

char *DAT_800a1f34[7] = {
	"PREVIOUS CINEMATICS",
	"KAREEM CAMPBELL",
	"ANDREW REYNOLDS",
	"RUNE GLIFBERG",
	"JAMIE THOMAS",
	"ELISSA STEAMER",
	"BAILS",
};

int DAT_800a1fd0[8] = {
	0x67827382, 0x72826f82, 0x62824081, 0x71826082, 0x64826482, 0x40817182, 0x40815b81,
};

int DAT_800a1ff0[7] = {
	0x67827382, 0x72826f82, 0x75824081, 0x63826882, 0x6e826482, 0x5b814081, 0x4081,
};

PsxVtEntry DAT_800a2120[2] = {
	{0, 0, 0},
	{0, 0, FUN_80066180},
};

PsxVtEntry DAT_800a2130[2] = {
	{0, 0, 0},
	{0, 0, dtor_8006553c},
};

PsxVtEntry DAT_800a2450[2] = {
	{0, 0, 0},
	{0, 0, FUN_80071a14},
};

PsxVtEntry DAT_800a2460[2] = {
	{0, 0, 0},
	{0, 0, FUN_80071acc},
};

PsxVtEntry DAT_800a2498[2] = {
	{0, 0, 0},
	{0, 0, FUN_80072e60},
};

PsxVtEntry DAT_800a24a8[2] = {
	{0, 0, 0},
	{0, 0, FUN_80072e94},
};

void *D_800a3630[1] = {
	Delay,
};

/* THPS2 demo: GamePreferences */
int GPreferences[104];

/* THPS2 demo: SCrowd[1] */
int Crowd[6];

/* SVideoRestart (THPS2 demo: SVideoRestart) */
int GVideoRestart[5];

/* SStats[11] (THPS2 demo: SStats[14]) */
SStats Stats[11] = {
	{
		"hawk", "hawk", "hawk_fe", 0, 0, 1, 2, 18, 8, {}, {3, 7, 7, 4}, "Tony Hawk",
		{"31", "USA", "Carlsbad", "16", "Goofy", "6'2"}, 0x1e64cc11, 0x59bd3dd8, 0x52158ad3,
		{{"b1hawk02.bmp", "Falcon 2"}, {"b1bird05.bmp", "B Logo"}, {"b1hawk03.bmp", "Evil Cat 2", 3, {1, 1, 1, 1}}, {"b1hawk07.bmp", "Full Skull 99", 7, {2, 1, 1, 2}}, {"b1bird09.bmp", "The End", 10, {3, 2, 2, 2}}, {"b1hawk04.bmp", "Falcon", 15, {5, 2, 2, 4}}, {"b1hawk01.bmp", "Evil Cat", 21, {6, 3, 3, 5}}, {"b1hawk08.bmp", "Falcon 3", 24, {7, 3, 3, 6}}},
	},
	{
		"burnq", "burnq_2p", "burnq_fe", 0x1000, 16, 17, 18, 12, 251, {}, {4, 6, 6, 4, 4},
		"Bob Burnquist", {"22", "Brazil", "Encinitas", "7", "Regular", "5'11"}, 0x19cbb6a2, 0x5eb19fcc,
		0x9989bf40,
		{{"b1burn10.bmp", "Brazil"}, {"b1burn02.bmp", "Stencil Logo"}, {"b1burn08.bmp", "Bob La Familia", 3, {1, 1, 1, 1}}, {"b1burn05.bmp", "Ribbon", 7, {2, 2, 2, 2}}, {"b1burn13.bmp", "Team United", 10, {3, 2, 2, 3}}, {"b1burn03.bmp", "Bob Stencil Logo", 15, {4, 3, 3, 4}}, {"b1burn12.bmp", "The Firm Team", 21, {5, 3, 3, 5}}, {"b1burn07.bmp", "Team La Familia", 24, {6, 4, 4, 6}}},
	},
	{
		"rowley", "rowley2p", "rowley_f", 0x2000, 16, 17, 18, 12, 243, {}, {7, 5, 3, 6, 1},
		"Geoff Rowley", {"23", "UK", "Huntington B.", "5", "Regular", "5'8"}, 0xe48875c7, 0xfe601e03,
		0x3fe2de21,
		{{"b1rowl01.bmp", "Industrial"}, {"b1rowl02.bmp", "Magic Lamp"}, {"b1rowl03.bmp", "Punk Skull", 3, {1, 1, 1, 1}}, {"b1rowl04.bmp", "Panic Attack", 7, {1, 2, 2, 2}}, {"b1rowl05.bmp", "Magic Cat", 10, {2, 2, 3, 2}}, {"b1rowl06.bmp", "White Trash", 15, {2, 3, 5, 3}}, {"b1rowl08.bmp", "Bull", 21, {3, 4, 6, 3}}, {"b1rowl07.bmp", "Rocky", 24, {3, 5, 7, 4}}},
	},
	{
		"lasek", "lasek_2p", "lasek_fe", 0x3000, 16, 17, 18, 12, 250, {}, {5, 7, 6, 3}, "Bucky Lasek",
		{"26", "USA", "Carlsbad", "9", "Regular", "5'11"}, 0x27aea97d, 0xa85230e0, 0x8c72375,
		{{"b1lase01.bmp", "Bird"}, {"b1bird04.bmp", "B Logo"}, {"b1lase03.bmp", "Flame Bird", 3, {1, 1, 1, 1}}, {"b1bird07.bmp", "The End", 7, {2, 1, 2, 2}}, {"b1bird06.bmp", "Team Seal", 10, {2, 2, 2, 3}}, {"b1lase04.bmp", "Bird Head", 15, {3, 2, 3, 5}}, {"b1bird10.bmp", "Birdhouse", 21, {4, 3, 3, 6}}, {"b1lase05.bmp", "Cat Killer", 24, {5, 3, 4, 7}}},
	},
	{
		"muska", "muska_2p", "muska_fe", 0x4000, 17, 18, 18, 13, 251, {}, {6, 4, 4, 7, 1}, "Chad Muska",
		{"22", "USA", "Los Angeles", "5", "Regular", "5'10"}, 0x3c18fee7, 0xeb2666c5, 0xf29f6d93,
		{{"b1musk05.bmp", "Large Red"}, {"b1shor02.bmp", "Medium Shortys"}, {"b1shor05.bmp", "Big Poppa", 3, {1, 1, 1, 1}}, {"b1musk02.bmp", "XX Large Green", 7, {2, 2, 2, 1}}, {"b1shor04.bmp", "Large Shortys", 10, {2, 3, 3, 2}}, {"b1shor06.bmp", "Westsider", 15, {3, 4, 4, 2}}, {"b1musk03.bmp", "X Large Blue", 21, {3, 5, 5, 3}}, {"b1musk04.bmp", "Muska Thugsta", 24, {4, 6, 6, 3}}},
	},
	{
		"campb", "campb_2p", "campb_fe", 0x5000, 16, 17, 18, 12, 249, {}, {7, 4, 4, 6, 1},
		"Kareem Campbell", {"25", "USA", "Los Angeles", "6", "Regular", "6'"}, 0x4cb23ba0, 0x2eb016a9,
		0x48bbe151,
		{{"b1camp07.bmp", "Rock"}, {"b1camp02.bmp", "Assassins"}, {"b1camp05.bmp", "CS Logo", 3, {1, 1, 1, 1}}, {"b1camp03.bmp", "City Stars", 7, {1, 2, 2, 2}}, {"b1camp06.bmp", "Stealth", 10, {2, 3, 3, 2}}, {"b1camp01.bmp", "Abalists 2000", 15, {2, 4, 4, 3}}, {"b1camp04.bmp", "Kareem Campbell", 21, {3, 5, 5, 3}}, {"b1camp08.bmp", "Racer", 24, {3, 6, 6, 4}}},
	},
	{
		"rynld", "rynld_2p", "rynld_fe", 0x6000, 0, 1, 2, 18, 7, {}, {6, 5, 3, 7, 1}, "Andrew Reynolds",
		{"20", "USA", "Huntington B.", "3", "Regular", "6'2"}, 0xdb6c5180, 0xf557cca, 0x46074169,
		{{"b1reyn08.bmp", "Monkey"}, {"b1bird11.bmp", "Birdhouse"}, {"b1reyn09.bmp", "Axe Man", 3, {1, 1, 1, 1}}, {"b1reyn01.bmp", "Reaper 2", 7, {2, 2, 2, 1}}, {"b1bird13.bmp", "B Logo", 10, {2, 2, 3, 2}}, {"b1reyn03.bmp", "Executioner", 15, {3, 3, 5, 2}}, {"b1bird08.bmp", "The End", 21, {3, 4, 6, 3}}, {"b1reyn04.bmp", "Reaper", 24, {4, 5, 7, 3}}},
	},
	{
		"glif", "glif_2p", "glif_fe", 0x7000, 16, 17, 18, 12, 248, {}, {4, 7, 7, 3, 3}, "Rune Glifberg",
		{"25", "Denmark", "Costa Mesa", "7", "Regular", "5'11"}, 0x74785085, 0xbaf82c20, 0xfbfdd452,
		{{"b1glif01.bmp", "Psycho"}, {"b1glif02.bmp", "Flip HKD"}, {"b1glif03.bmp", "Lil Vikings", 3, {1, 1, 1, 1}}, {"b1glif04.bmp", "Atomic 2", 7, {2, 1, 1, 2}}, {"b1glif05.bmp", "Snake Eyes", 10, {3, 2, 2, 3}}, {"b1glif06.bmp", "Purple Haze", 15, {4, 2, 2, 5}}, {"b1glif07.bmp", "Easyrider", 21, {5, 3, 3, 6}}, {"b1glif08.bmp", "Skunk", 24, {6, 3, 3, 7}}},
	},
	{
		"thomas", "thomas2p", "thomas_f", 0x8000, 16, 17, 18, 12, 251, {}, {5, 5, 4, 7, 1},
		"Jamie Thomas", {"24", "USA", "Encinitas", "6", "Regular", "5'10"}, 0x19b1a71f, 0xa197689b,
		0x90b3e1b3,
		{{"b1thom04.bmp", "Mug Shot"}, {"b1zero02.bmp", "Medium Bold"}, {"b1zero06.bmp", "Bloody Nose", 3, {1, 1, 1, 1}}, {"b1zero05.bmp", "American Zero", 7, {2, 2, 2, 1}}, {"b1zero08.bmp", "Bird", 10, {2, 2, 3, 2}}, {"b1zero07.bmp", "Small Skull", 15, {3, 3, 4, 2}}, {"b1thom03.bmp", "Skeleton", 21, {4, 4, 5, 3}}, {"b1thom01.bmp", "Smith Grind", 24, {5, 5, 6, 3}}},
	},
	{
		"steamr", "steamr2p", "steamr_f", 0x9000, 16, 17, 18, 15, 0, {}, {6, 4, 5, 6, 1},
		"Elissa Steamer", {"undisclosed", "USA", "Fort Myers", "1", "Regular", "5'4"}, 0x87caf65c,
		0xda8162ba, 0x4d46a154,
		{{"b1stea02.bmp", "Pushin Up"}, {"b1stea07.bmp", "Monster"}, {"b1stea01.bmp", "Griffin", 3, {1, 1, 1, 1}}, {"b1stea04.bmp", "Diamond Wizard", 7, {2, 2, 2, 2}}, {"b1stea08.bmp", "Small Fists", 10, {3, 3, 2, 2}}, {"b1stea03.bmp", "Steamer Cat", 15, {3, 4, 3, 3}}, {"b1stea09.bmp", "Throwin Goats", 21, {3, 5, 4, 3}}, {"b1stea05.bmp", "Stubborn Wood", 24, {4, 6, 5, 4}}},
	},
	{
		"fatcop", "fatcop", "fatcop_f", 0xa000, 16, 17, 18, 12, 251, {}, {5, 5, 5, 5, 2}, "Officer Dick",
		{"43", "USA", "Mallwood", "25", "Regular", "5'11"}, 0xfb7cfa8b, 0xa1cfa04a, 0x956362bc,
		{{"b1offi04.bmp", "Got Pork"}, {"b1offi06.bmp", "American Way"}, {"b1offi02.bmp", "Double Dunkin", 3, {1, 1, 1, 1}}, {"b1offi07.bmp", "Nightstick", 7, {2, 2, 2, 2}}, {"b1offi03.bmp", "Disaster", 10, {3, 2, 3, 3}}, {"b1offi08.bmp", "On the Beat", 15, {4, 3, 3, 4}}, {"b1offi05.bmp", "3 Course Meal", 21, {4, 4, 4, 5}}, {"b1offi01.bmp", "Game Over", 24, {5, 5, 5, 5}}},
	},
};

/* SLevel[10] (THPS2 demo: SLevel[10]) */
SLevel Levels[10] = {
	{
		"Warehouse", "Woodland Hills", "ware", 0, 0, "SkWare_T", 0, 0, 0, "levsel0.bmp", {136, 358},
		"load_war.bmp", "back_war.bmp", "boxes", 0, 0, 5000, 15000, 0, {50000, 50000},
	},
	{
		"School", "Miami", "SCHOOL", 0, 0, "SkSchl_T", 0, 0, 0, "levsel1.bmp", {334, 397}, "load_sch.bmp",
		"back_sch.bmp", "lunch tables", 2, 0, 7500, 25000, 0, {0, 30000}, {}, 1,
	},
	{
		"Mall", "New York", "MALL", 0, 1, "SkMall_T", 0, 0, 0, "levsel5.bmp", {338, 326}, "load_mal.bmp",
		"back_mal.bmp", "mall directories", 5, 0, 10000, 30000, 0, {}, {}, 1,
	},
	{
		"Skate Park", "Chicago", "COMP", 1, 0, "skvans_t", 15000, 10000, 5000, "levsel3.bmp", {280, 326},
		"loadcp1.bmp", "bckgrnd3.bmp", "objects", 8, 0, 12000, 30000, 1, {60000, 60000}, {}, 1, -3500,
		-1500, 900, 0, 2048, 300,
	},
	{
		"Downtown", "Minneapolis", "DOWNTOWN", 0, 1, "SkDown_T", 0, 0, 0, "levsel2.bmp", {254, 317},
		"load_dow.bmp", "back_dow.bmp", "signs", 0, 1, 15000, 40000, 0, {}, {}, 1,
	},
	{
		"Downhill Jam", "Phoenix", "JAM", 0, 1, "SkJam_T", 0, 0, 0, "levsel4.bmp", {167, 362},
		"load_dhj.bmp", "back_dhj.bmp", "valves", 13, 0, 20000, 40000, 0, {}, {}, 1,
	},
	{
		"Burnside", "Portland", "COMP", 1, 1, "SkBurn_T", 25000, 20000, 15000, "levsel8.bmp", {144, 297},
		"loadcp2.bmp", "bckgrnd4.bmp", "objects", 18, 0, 22000, 40000, 1, {80000, 70000}, {}, 1, -3500,
		-1200, 1500, 0, -2024, 30,
	},
	{
		"Streets", "San Francisco", "SF", 0, 1, "SkSF_T", 0, 0, 0, "levsel7.bmp", {130, 339},
		"load_san.bmp", "back_san.bmp", "cop cars", 0, 2, 25000, 50000,
	},
	{
		"Roswell", "New Mexico", "ROSWELL", 1, 1, "SkRos_T", 35000, 30000, 25000, "levsel9.bmp",
		{209, 368}, "loadcp3.bmp", "bckgrnd5.bmp", "objects", 26, 0, 30000, 40000, 0, {60000, 60000}, {},
		1, -3500, -1500, 900, 0, 2048, 300,
	},
	{"", "", "", 0, 1, "", 0, 0, 0, "", {}, "", "", 0, 0, 0, 0, 0, 0, {}, {0, 1}},
};

/* EFileType[15] (THPS2 demo: EFileType[15]) */
int FileType[15];

/* DIRENTRY[15] (THPS2 demo: DIRENTRY[15]) */
char SLOT_DIR[15][40];

/* SSaveGame (THPS2 demo: SSaveGame) */
int LastGameState[34];

/* SSaveGame (THPS2 demo: SSaveGame) */
int StartupGameState[34];

/* LocalText (THPS2 demo: LocalText) */
LocalText GameText = {
	"Tape Challenges", "End Game", "End Run", "No replay files", "No career files", "Save Career?",
	"A file by that", "name already exists", "S-K-A-T-E TAPE!", "HI SCORE TAPE!", "PRO SCORE TAPE!",
	"5 %s TAPE!", "HIDDEN TAPE!", "Enter name", "Press start", "Career mode", "Single session",
	"Two players", "Practice", "Autokick", "On", "Off", "Ollie", "Full Replay", "Highlights",
	"Save Replay", "Load Replay", "Exit", "Save Career", "Retry", "Restart Competition",
	"Level Select", "Equipment Select", "Watch replay", "Main Menu", "You got a tape!",
	"You got %d tapes!", "Statistics", "Total Score", "High Score", "Tapes Collected", "Tricks Landed",
	"Bails", "Best Combo", "None", "Best Combo - %d Tricks - %d Points",
	"Best Combo - %d Trick - %d Points", "Are you sure?", "Car plant", "Next Heat", "End Competition",
	"load completed", "save completed", "load failed", "save failed", "grind/slide", "grab trick",
	"flip trick", "flip trick/kick", "jump/ollie", "on <off>", "<on> off", "trick tips",
	"score display", "vibration", "the memory card in", "memory card slot 1", "is unformatted",
	"Formatting memory card", "Format failed", "Try again or insert", "A new memory card",
	"Format complete", "Return to competition", "Scroll %s for more choices", "up", "down",
	"up or down", "blue steals!", "red steals!", "%s high scores", "Play to:", "Change word",
	"Sound Levels", "SFX level: %d", "Music level: %d", "Skip track", "Back", "You get the letter",
	"Update save file", "Out of bounds!", "Nice one!", "You suck!", "Don't do drugs!",
	"X to exit   Square to pause   Circle to frame adv", "Paused", "Continue", "Save game ?", "Quit",
	"Please reinsert controller", "enter cheat", "go back", "select", "accept", "select level",
	"skip to restart", "accept", "cancel", "press any button", "yes", "no", "really quit ?", "options",
	"Empty", "Please insert a memory", "card in memory card", "slot 1 or press any",
	"button to cancel", "The memory card in", "memory card slot 1 is", "unformatted. Do you",
	"want to format it ?", "Are you sure you wish to", "format the memory card",
	"in memory card slot 1 ?", "formatting memory card", "do not remove", "memory card",
	"checking memory card", "reading memory card", "your memory card in", "memory card slot 1 is full",
	"insert a memory card with", "at least %d free block%s or delete", "%d block%s using the consoles",
	"internal memory card manager", "Select save slot to save", "to and press any button",
	"Okay to overwrite", "previously saved game", "in this save slot ?", "Now Saving data",
	"save failed", "format failed", "format completed", "memory card error", "select game to load",
	"insert memory card with", "saved game in memory", "card slot 1 or press any", "button to cancel",
	"T", "tape", "E", "S", "K", "A",
};

/* THPS2 demo: SMessageProg[5] */
int Messprog_Goal[40] = {
	0x10005, 0x1800000, -1, 0xffff, 0x1000000, 0x100005a, 90, 0x10001, 0x10001, 0x1800180, -1,
	0xffff, 0x1000000, 0x100005a, 90, 0x10001, 0x10004, 0x1000180, -1, 0xffff, 0x1000000, 0x100005a,
	90, 0x10001, 0x1001e, 0x1000100, -1, 0xffff, 0x1000000, 0x100005a, 90, 0x10001, 0x1000f, 256,
	-1, 0xffff, 0x1000000, 0x100005a, 90, 1,
};

int DAT_800a6388[24];

/* SMovieDetails[27] (THPS2 demo: SMovieDetails[30]) */
SMovieDetails movieDetails[27] = {
	{"\\ATVILOGO.STR;1", 320, 240, 208, 0, 153600, 48, 1, 1, 128},
	{"\\LOGO.STR;1", 320, 144, 237, 0, 92160, 48, 1, 1, 128},
	{"\\SUICIDE.STR;1", 96, 64, 3667, 0, 0x4000, 16, 0, 1, 102},
	{"\\GOLDFING.STR;1", 96, 64, 3746, 0, 0x4000, 16, 0, 1, 102},
	{"\\SPEED.STR;1", 96, 64, 3777, 0, 0x4000, 16, 0, 1, 76},
	{"\\DK.STR;1", 96, 64, 3751, 0, 0x4000, 16, 0, 1, 102},
	{"\\PRIMUS.STR;1", 96, 64, 3755, 0, 0x4000, 16, 0, 1, 204},
	{"\\UNSANE.STR;1", 96, 64, 3746, 0, 0x4000, 16, 0, 1, 102},
	{"\\VANDALS.STR;1", 96, 64, 3746, 0, 0x4000, 16, 0, 1, 102},
	{"\\ST.STR;1", 96, 64, 3749, 0, 0x4000, 16, 0, 1, 102},
	{"\\BAILS.STR;1", 320, 240, 790, 0, 153600, 48, 1, 1, 128},
	{"\\HAWK.STR;1", 320, 240, 464, 0, 153600, 48, 1, 1, 128},
	{"\\BURNQUIS.STR;1", 320, 240, 466, 0, 153600, 48, 1, 1, 128},
	{"\\ROWLEY.STR;1", 320, 240, 454, 0, 153600, 48, 1, 1, 128},
	{"\\LASEK.STR;1", 320, 240, 488, 0, 153600, 48, 1, 1, 128},
	{"\\MUSKA.STR;1", 320, 240, 452, 0, 153600, 48, 1, 1, 128},
	{"\\CAMPBELL.STR;1", 320, 240, 495, 0, 153600, 48, 1, 1, 128},
	{"\\REYNOLDS.STR;1", 320, 240, 462, 0, 153600, 48, 1, 1, 128},
	{"\\GLIFBERG.STR;1", 320, 240, 477, 0, 153600, 48, 1, 1, 128},
	{"\\THOMAS.STR;1", 320, 240, 477, 0, 153600, 48, 1, 1, 128},
	{"\\STEAMER.STR;1", 320, 240, 437, 0, 153600, 48, 1, 1, 128},
	{"\\AARON.STR;1", 320, 240, 619, 0, 153600, 48, 1, 1, 128},
	{"\\EVENRUDE.STR;1", 96, 64, 3751, 0, 0x4000, 16, 0, 1, 128},
	{"\\ERNIES.STR;1", 96, 64, 3750, 0, 0x4000, 16, 0, 1, 102},
	{"\\INTRO.STR;1", 320, 240, 991, 0, 153600, 48, 1, 1, 128},
	{"\\411.STR;1", 320, 240, 451, 0, 153600, 48, 1, 1, 128},
	{0},
};

/* the movies' order (FUN_80022b4c), which retail kept in the empty 27th movieDetails entry */
unsigned char MovieOrder[30] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
	26, 27, 28, 29,
};

/* the viewports (SViewport; the code sets TanHalf by its own name, <name>_c, before a game) */
SViewport ApocalypseViewport = {512, 240, 0, 0, 10, 30000, 0};
SViewport LeftViewport = {254, 240, 0, 0, 10, 30000, 0};
SViewport RightViewport = {512, 240, 258, 0, 10, 30000, 0};
SViewport TopViewport = {512, 118, 0, 0, 10, 30000, 0};
SViewport BottomViewport = {512, 240, 0, 122, 10, 30000, 0};
SViewport TopLeftViewport = {256, 120, 0, 0, 10, 30000, 0};
SViewport BottomRightViewport = {512, 240, 256, 120, 10, 30000, 0};
SViewport TopLeftBigViewport = {304, 152, 0, 0, 10, 30000, 3413};
SViewport BottomRightBigViewport = {512, 240, 208, 88, 10, 30000, 0};

/* SCareer (THPS2 demo: SCareer) */
int GCareer[628];

int DAT_800a7214[3];

int DAT_800a7f28[3];
int DAT_800a7f34[3];

SStats DAT_800a7f40 = {
	"skater", "skater", "skater_f", 0xb000, 16, 17, 18, 15, 251, {}, {10, 10, 10, 10, 2},
	"Private Carrera", {"Barely Legal", "USA", "Fort Worth", "1", "Regular", "5'8"}, 0xbfa29d29,
	0x43d4141b, 0xda362762,
	{{"b1neve08.bmp", "Erector Set"}, {"b1neve07.bmp", "Viagra Falls"}, {"b1neve03.bmp", "Manhandler", 3}, {"b1neve05.bmp", "Feelin Blue", 7}, {"b1neve02.bmp", "New Member", 10}, {"b1neve06.bmp", "Solid Wood", 15}, {"b1neve04.bmp", "She Got Pop", 21}, {"b1neve01.bmp", "Skate Hard", 24}},
};

STrick DAT_800a8094 = {
	0x80b421, 62, 0x10000, "{SOMI SPIN}", 0, 5, 0, 5500, 0, 128,
};

STrick DAT_800a80c0 = {
	0x200b413, 63, 0x10000, "{HO-HO HO}", 0, 10, 10, 2000, 0, 128,
};

STrick DAT_800a80ec = {
	0xb842, 64, 0x10000, "{THE WELL HARDFLIP}", 0, 10, 10, 5500,
};

unsigned char DAT_800a8245[7];

/* CVector (THPS2 demo: CVector) */
int ZeroVector[3];

int D_800b8a74[3] = {
	128, 0x26c0, 0x1f40,
};

unsigned char D_800b8a80[2] = {0x40, 0x48};

int D_800b8a84[6] = {
	0x6fe0, 0xade0, 0xf6c0, 0x18040, 0x18040, 0x3c00,
};

int DAT_800b8a9c[350] = {
	0x80000001, 100, 0x80000002, 100, 4, 0x3c0064, 8, 100, 16, 0x3f0064, 32, 100, 64, 0x3c0064,
	0x80000080, 0, 256, 200, 512, 400, 1024, 400, 0x80010001, 100, 0x80010002, 200, 0x80010004,
	0x3c0096, 0x80010008, 100, 0x80010010, 100, 0x10008, 200, 0x20001, 100, 0x20002, 500, 0x20004,
	400, 0x20008, 500, 0x20008, 300, 0x20010, 80, 0x20010, 100, 0x30001, 300, 0x30002, 200, 0x30004,
	200, 0x30008, 200, 0x40001, 100, 0x40002, 100, 0x40004, 100, 0x40008, 0x3a0064, 0x40010, 100,
	0x80050001, 100, 0x50002, 100, 0x50004, 100, 0x50008, 100, 0x50010, 100, 0x50020, 120, 0x50040,
	120, 0x50080, 100, 0x60001, 100, 0x70001, 200, 0x70002, 200, 0xc0000001, 0, 0xc0000002, 100,
	0xc0000004, 100, 0x40000008, 100, 0x40000010, 100, 0x40000020, 100, 0x40000040, 100, 0x40000080,
	100, 0x40000100, 70, 0x40010001, 100, 0xc0020001, 100, 0xc0030001, 100, 0xc0040001, 100,
	0xc0000001, 100, 0x40000002, 300, 0xc0000004, 100, 0x40000008, 100, 0xc0000010, 100, 0xc0000020,
	300, 0x40000040, 100, 0x40000080, 100, 0x40000100, 300, 0x40000200, 100, 0x40000400, 100,
	0x40000800, 100, 0x40001000, 100, 0xc0000002, 100, 0xc0010001, 100, 0xc0000002, 100, 0xc0000004,
	100, 0xc0000008, 100, 0xc0000010, 500, 0xc0000020, 300, 0xc0000040, 300, 0xc0000080, 300,
	0x40010001, 100, 0x40010002, 100, 0x40010004, 100, 0x40010008, 100, 0x40010010, 60, 0x40010020,
	60, 0x40010040, 400, 0x40010080, 250, 0x40010100, 300, 0x40010200, 300, 0xc0020001, 500,
	0xc0030001, 0x4600c8, 0x40000001, 100, 0x40000004, 100, 0x40000008, 0x3c01f4, 0x40000010, 300,
	0xc0000020, 300, 0xc0000040, 300, 0xc0000080, 100, 0x40000100, 500, 0x40000200, 500, 0x40000400,
	300, 0x40000800, 300, 0x40002000, 70, 0x40004000, 70, 0x40008000, 70, 0xc0010001, 100,
	0x40000200, 500, 0x40000400, 500, 0x40000800, 120, 0x40001000, 100, 0x40010001, 100, 0x40000040,
	100, 0x40000080, 100, 0xc0000100, 0x32004b, 0x40010010, 200, 0x40010020, 500, 0x40010040, 500,
	0x40010400, 500, 0x40010800, 500, 0xc0050001, 100, 0x40000008, 100, 0xc0000004, 100, 0x40000020,
	100, 0x40000040, 100, 0x40000080, 200, 0x40000004, 200, 0x40000100, 200, 0x40000800, 200,
	0x40001000, 200, 0xc0010001, 100, 0x40010002, 100, 0x40010004, 100, 0xc0010008, 100, 0xc0010010,
	100, 0xc0020001, 100, 0x40000200, 100, 0x40010001, 100, 0x40010002, 200, 0x40010004, 300,
	0xc0010008, 60, 0xc0020001, 300, 0xc0020002, 100, 0xc0020004, 170, 0xc0020008, 100, 1, 100, 2,
	100, 4, 100, 8, 100, 16, 100, 32, 100, 64, 100, 128, 100, 0x40000001, 100, 0x40000004, 100,
	0x40000008, 100, 0x40000010, 200, 0x40000020, 200, 0x10001, 500, 0x10002, 250, 0x20020, 300,
	0x40000200, 300, 0x20040, 300, 0x20080, 400, 0x40010010, 100, 0x40010020, 90, 0xc0010040, 100,
	0x40010080, 200, 0x40010100, 200, 0x40010200, 80, 0xc0060001, 100, 0xc0020010, 100, 0xc0030001,
	100, 0x10004, 250, 0x10008, 250, 0x10010, 250,
};

int DAT_800b9014[34];

/* THPS2 demo: uchar[256] */
unsigned char BigFont_Mapping[256] = {
	0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0xff, 0x2c, 0x2a, 0x2a, 0x2a, 0x02, 0x2a, 0x30, 0x2a, 0x2a, 0x2a, 0x01, 0x2a, 0x32, 0x31, 0x2a,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2a, 0x2a, 0x35, 0x2a, 0x2a,
	0x2a, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x2a, 0x2a, 0x2a, 0x2a, 0x00,
	0x2a, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x14, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2b, 0x2a, 0x2a, 0x2a, 0x03, 0x2a, 0x2a, 0x2a, 0x2e, 0x2d, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2f, 0x2a, 0x04, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x05, 0x2a, 0x2a, 0x2a,
	0x2b, 0x2a, 0x2a, 0x2a, 0x03, 0x2a, 0x2a, 0x2a, 0x2e, 0x2d, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a,
	0x2a, 0x2a, 0x2a, 0x2a, 0x2f, 0x2a, 0x04, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x05, 0x2a, 0x2a, 0x2a,
};

/* the heaps (game/mem.h) */
SHeapRange DAT_800be09c[2];
int DAT_800be0ac[96];

int DAT_800be3d0[10] = {
	-6, -24, -23, -4, -7, -5, -10, -3, -8, -9,
};

int DAT_800be3f8[13] = {
	0x10000000, 0, 0xf000, 4096, 0, 0x2c003fc, 0x2840dc0, 0xdc002c0, 0x2c001a0, 3520, 1024, 1024,
	1024,
};

int DAT_800be444[13] = {
	0x10000000, 0, 0xf000, 0, 4096, 0xb800000, 0, 2944, 0xb800000, 0, 3136, 3136, 3136,
};

SScreenFiles DAT_800be478[15] = {
	{{"main_h.bmp", "mainf.fnt", "selectp.bmp"}, 3},
	{{"selectp.bmp", "select2p.bmp", "Player1.pre"}, 3},
	{{"select2p.bmp", "Player2.pre"}, 2},
	{{"select_c.bmp", "Level.pre"}, 2},
	{{"Pressx.bmp", "check2.bmp"}, 2},
	{{"loadingt.bmp", "Pressx.bmp", "Bckgrnd1.bmp"}, 3},
	{{"Pressx.bmp"}, 1},
	{{"loadingg.bmp", "Pressx.bmp", "Bckgrnd1.bmp"}, 3},
	{{"loadingv.bmp", "Bckgrnd2.bmp"}, 2},
	{{"select_b.bmp", "Equip1.pre"}, 2},
	{{"select2b.bmp", "Equip2.pre"}, 2},
	{{"options.bmp"}, 1},
	{{"opt_snd.bmp"}, 1},
	{{"opt_ctr.bmp"}, 1},
	{{"load_hor.bmp", "Pressx.bmp", "Bckgrnd1.bmp"}, 3},
};

/* a FileLoader (the front end's, FrontLoader_*) */
PsxStorage<FileLoader> DAT_800bec34;

int DAT_800bf0dc[1] = {
	44,
};

int DAT_800bf0e8[1] = {
	184,
};

int DAT_800bf0ec[1] = {
	57,
};

int DAT_800bf0f0[1] = {
	56,
};

int DAT_800bf0f4[1] = {
	56,
};

int DAT_800bf0f8[1] = {
	206,
};

int DAT_800bf0fc[1] = {
	204,
};

int DAT_800bf100[1] = {
	214,
};

/* DAT_800bf104 (a PadBuffer, zero until its static constructor builds it): src/frontend.cpp (FUN_8005de08) */
SlicedImage *DAT_800bf2f0[10];

/* GVInfo2 (0x800bf3d8) is GVInfo1[1] (data/video.cpp) */

int DAT_800bf614[16] = {
	0x1e00280, 0, 0x7530000a, 3413,
};

/* SPSXRegion[40] (THPS2 demo: SPSXRegion[40]) */
SPSXRegion PSXRegion[40];


/* THPS2 demo: uint*[40] */
uint *Memories[40];

/* SControl[2] (THPS2 demo: SControl[2]) */
SControl Pad[2];

/* THPS2 demo: CVector */
PsxQuat UpDownPosition_8;

/* CQuat (THPS2 demo: CQuat) */
PsxQuat testquat2;

/* CQuat (THPS2 demo: CQuat) */
PsxQuat testquat3;

/* SLight (THPS2 demo: SLight) */
int M3d_DefaultLight[13] = {
	0x10000000, 0, 0xf000, 0, 4096, 0x9001000, 0x8000800, 0x8000d00, 0x10000200, 2048, 512, 512,
	512,
};

/* THPS2 demo: SVertex[800]. ProjectedVertices, with RotatedVertices 3200 bytes on; lighting
   writes a RotatedVertices word for each of a model's normals (vertices + faces), which overran
   the 6400 bytes with the Dreamcast's bigger models, so the tail has room for any u16 count */
int Vertices[(3200 + 65536 * 8) / 4];

/* THPS2 demo: short[1024] */
short WibbleTables[1024];

unsigned char DAT_800cffff[1];
unsigned char DAT_800d0000[1];
unsigned char DAT_800d0001[3];
int DAT_800d0004[1];
short DAT_800d0008[1];
unsigned char DAT_800d000a[1];
int DAT_800d000c[1];
int DAT_800d0014[13];
int DAT_800d0048[1];
int DAT_800d004c[1];
int DAT_800d0050[12];
int DAT_800d0080[3];
int DAT_800d008c[29];

void *DAT_800d0100[24] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 /* (Psy-Q library data) */, 0, FUN_80096f18,
};

int DAT_800d10e8[1];

/* THPS2 demo: int */
int Crowd_Bored[1];

/* THPS2 demo: int */
int GHasCareerLoaded[1];

/* THPS2 demo: int */
int SelectedSkater[1];

int DAT_800d113c[1];
int DAT_800d1140[1];
int DAT_800d137c[1];

/* THPS2 demo: ulong */
unsigned int ev10[1];

/* THPS2 demo: ulong */
unsigned int ev11[1];

/* THPS2 demo: ulong */
unsigned int ev12[1];

/* THPS2 demo: ulong */
unsigned int ev13[1];

/* THPS2 demo: int */
int SFXLEVEL[1] = {
	0x4000,
};

/* THPS2 demo: int */
int XALEVEL[1] = {
	0x1f40,
};

void *DAT_800d1408;

/* THPS2 demo: int */
int GamePaused[1];

/* THPS2 demo: CMenu* */
CMenu *pYesNoMenu;

int DAT_800d1524[1];
int DAT_800d1528[1];
int DAT_800d152c[1];

int D_800d15c4[1] = {
	84,
};

int D_800d15d0[1] = {
	69,
};

int D_800d15d4[1] = {
	83,
};

int D_800d15d8[1] = {
	75,
};

int D_800d15dc[1] = {
	65,
};

/* THPS2 demo: int */
int GameFMV_Active[1];

/* THPS2 demo: int */
int CycleTracks[1] = {
	2,
};

int DAT_800d1650[1] = {
	4,
};

/* THPS2 demo: short */
short GameFMV_StartX[2];

/* THPS2 demo: int */
int GameFMV_NumSoundSources[1];

/* THPS2 demo: int */
int GameFMV_MusicScale[1];

int DAT_800d1694[1] = {
	1,
};

/* THPS2 demo: null */
int Redbook_XACheckSectorOnVSync[1];

int DAT_800d169c[1] = {
	1,
};

/* THPS2 demo: null */
int Redbook_XAModeSet[1];

/* THPS2 demo: int */
int GGame[1] = {
	1,
};

char *DAT_800d16dc;

/* THPS2 demo: int */
int TTime[1];

/* THPS2 demo: int */
int GameFrozen[1];

/* THPS2 demo: int */
int Cheat_PanelOn[1] = {
	1,
};

/* THPS2 demo: int */
int Cheat_FPS[1];

/* THPS2 demo: int */
int PolyBufferBytesUsed[1];

/* THPS2 demo: int */
int MusicStopped[1];

/* THPS2 demo: int */
int ViewportMode[3] = {
	0, 16, 150,
};

int DAT_800d1748[2] = {
	1,
};

int DAT_800d1750[1];

/* THPS2 demo: int */
int GNumberOfPlayers[1];

/* THPS2 demo: CBruce*[2] */
CBruce *GSkater;

/* THPS2 demo: int */
int XFramesShifted[1];

int DAT_800d1840[1];

/* THPS2 demo: int */
int LastXblanks[1];

/* THPS2 demo: int */
int Profile_Bop[1];

/* THPS2 demo: Sprite* */
Sprite *pSpecialSprite;

Sprite *DAT_800d1858[2];

/* THPS2 demo: int */
int UseCacheForBits[1] = {
	1,
};

/* THPS2 demo: CNonRenderedBit* */
void *NonRenderedBitList;

/* THPS2 demo: CFlatBit* */
void *FlatBitList;

/* THPS2 demo: CLinked2EndedBit* */
void *Linked2EndedBitList;

/* THPS2 demo: CPixel* */
void *PixelList;

/* THPS2 demo: CQuadBit* */
void *QuadBitList;

/* THPS2 demo: CGenPoly* */
void *GenPolyList;

/* THPS2 demo: CGlassBit* */
void *GlassList;

/* THPS2 demo: CPolyLine* */
void *PolyLineList;

/* THPS2 demo: CGPolyLine* */
void *GPolyLineList;

/* THPS2 demo: CBody* */
CBruce *MechList[2];

int DAT_800d19ac[1];
short DAT_800d19b8[1];
short DAT_800d19ba[1];
int DAT_800d19bc[1];

/* THPS2 demo: uint */
unsigned int SuspendDistance[1] = {
	0x32c8,
};

int DAT_800d1a18[1];

/* THPS2 demo: uint */
unsigned int Vblanks[1];

/* THPS2 demo: uint */
unsigned int Xblanks[1];

/* THPS2 demo: uint */
unsigned int LastVblanks[1];

/* THPS2 demo: int */
int DoDisplayDraw[1];

int DAT_800d1a38[1];

/* THPS2 demo: uint */
unsigned int Db_SkyColor[1];

int DAT_800d1a40[2] = {
	0, 0x1ff0200,
};

/* THPS2 demo: SDoubleBuffer* */
SDoubleBuffer *pDoubleBuffer;

/* THPS2 demo: SDoubleBuffer* */
SDoubleBuffer *pOtherBuffer;

/* THPS2 demo: uint* */
int OrderingTable[1];

/* THPS2 demo: uint* */
unsigned char *pPoly;

/* THPS2 demo: SDoubleBuffer* */
SDoubleBuffer *pVB[2];

SPalette *DAT_800d1a80;

int DAT_800d1a94[1] = {
	-1,
};

int DAT_800d1a98[1] = {
	-1,
};

/* THPS2 demo: null */
int SFXPaused[1];

/* THPS2 demo: uint */
unsigned int soundRAMOccupied[1];

/* THPS2 demo: int */
int SFX_v[1];

/* THPS2 demo: void* */
char *pSFXVABHeader;

/* THPS2 demo: void* */
char *pLevelSpecificSFXVABHeader;

/* THPS2 demo: SAnimFrame* */
SAnimFrame *BigFontAnim;

/* THPS2 demo: int */
int CurrentPos[1];

/* THPS2 demo: int */
int Loading[1];

/* THPS2 demo: char* */
char *FilePath = "m:\\psx\\sk\\?\??\??\??\?.?\??";

/* THPS2 demo: int */
int FileOpen[1];

/* THPS2 demo: int */
int SectorsPerFrame[1] = {
	4,
};

/* THPS2 demo: null */
PREManager *SpoolerBlocksFileIO;

/* THPS2 demo: int */
int LoadFromPRE[1];

/* THPS2 demo: int */
int LastLoading[1];

SBlockHeader *DAT_800d1b48;
SBlockHeader *DAT_800d1b4c;

/* THPS2 demo: uint */
unsigned int CriticalBigHeapUsage;

/* THPS2 demo: int[2] */
int Used[2];

/* THPS2 demo: CTrickOb* */
CTrickOb *TrickObList;

int DAT_800d1b74[6] = {
	0x2580708, 900, 256, 220, 128,
};

/* THPS2 demo: CRail* */
CRail *RailList;

int DAT_800d1b98[3] = {
	1, 1, 945,
};

int DAT_800d1ba8[1];

int DAT_800d1c0c[1] = {
	0x1b88,
};

int DAT_800d1c10[2] = {
	0x1b88,
};

int DAT_800d1c18[1] = {
	52,
};

TSPack *DAT_800d1c80;
TSPack *DAT_800d1c84;
TSPack *DAT_800d1c88;
TSPack *DAT_800d1c8c;
TSPack *DAT_800d1c90;
int DAT_800d1c94[1];
int DAT_800d1c98[1];
int DAT_800d1ca0[1];
int DAT_800d1ca4[1];
int DAT_800d1ca8[1];
int DAT_800d1cac[1];

/* THPS2 demo: int */
int DemoTimer[1];

int DAT_800d1cbc[1];
int DAT_800d1cc0[1];
int DAT_800d1cc4[1];
int DAT_800d1cd8[1];
int DAT_800d1cdc[1];
int DAT_800d1ce0[1];
int DAT_800d1ce4[1];
int DAT_800d1ce8[1];
int DAT_800d1cec[1];

/* THPS2 demo: int */
int BackOut[1];

int DAT_800d1cf4[1];
int DAT_800d1cf8[2];
Sprite *DAT_800d1d0c[2];
int DAT_800d1d14[1];
int DAT_800d1d18[3];
int DAT_800d1d28[1];
int DAT_800d1d2c[1];
Sprite *DAT_800d1d5c;
Sprite *DAT_800d1d60;
int DAT_800d1d7c[1];
int DAT_800d1d80[1];

int DAT_800d1d84[1] = {
	-1,
};

void *DAT_800d1d88;
short DAT_800d1d8c[1];
short DAT_800d1d8e[1];
short DAT_800d1d90[1];
short DAT_800d1d92[1];

/* THPS2 demo: int */
int OT_entry[1] = {
	4096,
};

int DAT_800d1d98[1];
int DAT_800d1d9c[1];

/* THPS2 demo: int */
int FirstEntry[1];

/* THPS2 demo: int */
int LastEntry[1];

/* THPS2 demo: int */
int TrickOb1[1];

/* THPS2 demo: int */
int TrickOb2[1];

int DAT_800d1e20[2] = {
	0, 0x2000280,
};

/* THPS2 demo: int */
int GLevel[1];

int DAT_800d1e48[1];
SAnimFrame *DAT_800d1e4c[2];

/* THPS2 demo: int */
int TAPES_Display[1];

int DAT_800d1ec4[1];

/* THPS2 demo: uint */
unsigned int LastHoldingScore[1];

/* THPS2 demo: null */
int prim_added[1];

int DAT_800d1ed8[1];
int DAT_800d1edc[1];

int DAT_800d1f18[7] = {
	0, 0x80646400, 0, 0, 3,
};

int DAT_800d1f34[1];

int DAT_800d1f38[1] = {
	255,
};

int DAT_800d1f3c[1];

int DAT_800d1f40[1] = {
	255,
};

int DAT_800d1f44[1];

int DAT_800d1f48[1] = {
	255,
};

int DAT_800d1f4c[1];

int DAT_800d1f50[1] = {
	255,
};

int DAT_800d1f54[1] = {
	128,
};

int DAT_800d1f58[1] = {
	128,
};

int DAT_800d1f5c[1] = {
	128,
};

int DAT_800d1f60[2] = {
	128,
};

void *DAT_800d1f78;

/* a RECT: all of VRAM */
short DAT_800d1f8c[4] = {0, 0, 1024, 512};


/* THPS2 demo: uchar */
unsigned char InShell[4];

/* THPS2 demo: CBody* */
CBody *CameraList;

short DAT_800d2068[1];
short DAT_800d206a[1];

int D_800d207c[1] = {
	1,
};

/* THPS2 demo: int */
int GHeadPart[1] = {
	-1,
};

/* EWaterClip (THPS2 demo: EWaterClip) */
int WaterClipValue[1] = {
	0xffff,
};

/* SVECTOR (THPS2 demo: SVECTOR) */
int FarColor[2];

/* THPS2 demo: int */
int TestTheWater[1];

void *DAT_800dc94c[1] = {
	0, /* (the end of the retail .bss: the Psy-Q heap) */
};
