/* Decompiled data: the retail executable's initialised globals that src/trig/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"

struct CBody;
struct CBruce;
struct STrig;
#include "game/panel.h"

extern char D_800a0820[];
extern char D_800a097c[];

/* SGapTrick[166] (THPS2 demo: SGapTrick[304]) */
SGapTrick GapTricks[166] = {
	{0, 250, "[WIMPY GAP]"},
	{1, 500, "[GAP]"},
	{2, 1000, "[PHAT GAP]"},
	{1001, 200, "[TRANSFER]"},
	{1005, 600, "[TAXI GAP]"},
	{1006, 100, "[KICKER GAP]"},
	{1007, 300, "[OVER THE PIPE]"},
	{1008, 300, "[SECRET ROOM]"},
	{1009, 400, "[FACEPLANT]"},
	{1010, 1000, "[ACID DROP]"},
	{1100, 100, "[KICKER 2 STREET]"},
	{1101, 500, "[BS GAP]"},
	{1102, 500, "[T 2 T GAP]"},
	{1103, 500, "[SECRET TUNNEL ENTRANCE]"},
	{1104, 1000, "[TUNNEL GAP]"},
	{1105, 2000, "[OVER THE TUNNEL]"},
	{1106, 100, "[CAR OLLIE]"},
	{1107, 50, "[CHEESY DECK GAP]"},
	{1108, 250, "[DECK GAP]"},
	{1109, 2500, "[BURLY DECK GAP]"},
	{1110, 250, "[TRUCK GAP]"},
	{1111, 2000, "[ROOF 2 ROOF]"},
	{1112, 1500, "[SUCKY ROOM GAP]"},
	{1113, 1500, "[BIG ASS]"},
	{1114, 750, "[GLASS GAP]"},
	{1200, 1000, "[WHOOP GAP]"},
	{1201, 100, "[WALL GAP]"},
	{1202, 100, "[OVER THE BOX]"},
	{1203, 2000, "[OVER THE RAFTERS]"},
	{1204, 700, "[OVER THE PIPE]"},
	{1205, 500, "[POOL HIP]"},
	{1206, 700, "[POOL 2 WALKWAY]"},
	{1207, 250, "[HP TRANSFER]"},
	{1300, 1000, "[BRIDGE GAP]"},
	{1301, 700, "[VERT WALL GAP]"},
	{1302, 700, "[TWINKIE TRANSFER]"},
	{1303, 800, "[OVER DA POOL]"},
	{2000, 500, (char *)D_800a0820},
	{2001, 1000, "[SWIM TEAM GAP]"},
	{2002, 125, "[AWNING GAP]"},
	{2003, 50, "[GARBAGE OLLIE]"},
	{2004, 750, "[ROOF TO AWNING GAP]"},
	{2005, 250, "[DITCH SLAP]"},
	{2006, 750, "[OVER THE AIR CONDITIONER]"},
	{2007, 1000, "[OVER A FOOTBRIDGE]"},
	{2008, 500, "[PARK GAP]"},
	{2009, 250, "[MINI GAP]"},
	{2010, 100, "[PLANTER GAP]"},
	{2011, 100, "[KICKER GAP]"},
	{2300, 250, "[OVER A 16 STAIR SET]"},
	{2301, 2000, "[OVER A HUGE 32 STAIR GAP]"},
	{2302, 500, "[]"},
	{2303, 500, "[SKATER ESCALATOR GAP]"},
	{2304, 2500, "[32 STEPS OFF A MEZZANINE]"},
	{2305, 100, "[THE FLYING LEAP]"},
	{2306, 100, "[PLANTER GAP]"},
	{2307, 250, "[RAMP GAP]"},
	{2308, 250, "[GOING UP GAP]"},
	{2309, 250, (char *)D_800a097c},
	{2310, 250, "[FOUNTAIN GAP]"},
	{2600, 250, "[RAGING GORGE GAP]"},
	{2602, 1000, "[HUGE WATER HAZARD GAP]"},
	{2603, 50, "[50 FEET]"},
	{2604, 100, "[100 FEET]"},
	{2605, 150, "[150 FEET]"},
	{2606, 200, "[200 FEET]"},
	{2607, 250, "[250 FEET]"},
	{2609, 250, "[SMALL WATER HAZARD GAP]"},
	{2610, 500, "[BIG WATER HAZARD GAP]"},
	{2611, 25, "[25 FEET]"},
	{2612, 75, "[75 FEET]"},
	{2613, 125, "[125 FEET]"},
	{2614, 175, "[175 FEET]"},
	{2615, 225, "[225 FEET]"},
	{3000, 250, "[RAMP 2 RAMP]"},
	{3001, 500, "[TUNNEL GAP]"},
	{3003, 500, "[LOW DECK GAP]"},
	{3004, 1000, "[HIGH DECK GAP]"},
	{3005, 1500, "[DECK GAP]"},
	{3300, 250, "[PORCH GAP]"},
	{3301, 500, "[PORCH GAP]"},
	{3302, 5000, "[LOMBARD GAP]"},
	{3303, 500, "[THE GONZ GAP]"},
	{3304, 1000, "[PAGODA GAP]"},
	{3305, 100, "[OVER THE SEVEN]"},
	{3306, 750, "[HUBBA GAP]"},
	{3307, 250, "[STREET GAP]"},
	{3308, 500, "[STREET GAP]"},
	{3309, 1000, "[HANDI GAP]"},
	{3310, 500, "[C BLOCK GAP]"},
	{3311, 500, "[PLANTER GAP]"},
	{3312, 750, "[FOUNTAIN GAP]"},
	{3313, 1000, "[SPINE GAP]"},
	{3314, 500, "[RAMP 2 RAMP]"},
	{3315, 750, "[RAMP 2 RAMP]"},
	{3316, 500, "[OVERSIZED 8 SET]"},
	{3317, 1000, "[ACID DROP-IN]"},
	{3318, 250, "[FOUNTAIN GAP"},
	{3319, 750, "[PORCH GAP]"},
	{3320, 500, "[KICKER GAP]"},
	{3321, 500, "[PAGODA HOP]"},
	{3322, 750, "[PAGODA HOP]"},
	{11000, 250, "[CHANNEL GAP]"},
	{11001, 200, "[KICKER 2 LEDGE]"},
	{11002, 1000, "[GAP]"},
	{11100, 100, "[FREAKIN POOL GRIND]"},
	{11101, 100, "[GAP]"},
	{11102, 100, "[GAP]"},
	{11200, 1000, "[ROLL IN CHANNEL GAP]"},
	{11201, 500, "[CHANNEL GAP]"},
	{12000, 1000, "[HALL PASS GAP]"},
	{12001, 1000, "[GAP]"},
	{12002, 1000, "[GAP]"},
	{13000, 1000, "[GAP]"},
	{13001, 1000, "[GAP]"},
	{13002, 1000, "[GAP]"},
	{21000, 200, "[BIG RAIL]"},
	{21001, 300, "[DECK 2 RAIL]"},
	{21002, 500, "[TAXI 2 LEDGE]"},
	{21003, 1000, "[TAXI 2 RAIL]"},
	{21004, 200, "[KICKER 2 LEDGE]"},
	{21005, 500, "[MONSTER GRIND]"},
	{21006, 200, "[HIGH RAIL]"},
	{21007, 3000, "[HOLY SHI...]"},
	{21008, 400, "[TRANSITION GRIND]"},
	{21200, 100, "[KICKER 2 EDGE]"},
	{21201, 200, "[BS GRIND]"},
	{21202, 750, "[RAIL 2 RAIL TRANSFER]"},
	{21203, 500, "[BILLBOARD GRIND]"},
	{21204, 3000, "[DIRTY RAIL]"},
	{21205, 2000, "[DEATH GRIND]"},
	{21300, 1000, "[TRIPLE RAIL]"},
	{21301, 800, "[BRIDGE GRIND]"},
	{21302, 1000, "[HAWK BRIDGE GRIND]"},
	{21400, 1000, "[RAFTER RAIL]"},
	{21401, 1000, "[PIPE 2 BOX GRIND]"},
	{21402, 500, "[LIGHT GRIND]"},
	{21403, 700, "[WALKWAY RAIL TRANS]"},
	{21404, 1000, "[POOL RAIL TRANS]"},
	{21500, 1000, "[ET GRIND]"},
	{21501, 1000, "[BHOUSE RAIL]"},
	{21502, 2000, "[POOL GRIND]"},
	{21503, 800, "[DECK GRIND]"},
	{0x5400, 2000, "[MB EMERSON GRIND]"},
	{22000, 250, "[DUMPSTER RAIL GAP]"},
	{22001, 500, "[PLAYGROUND RAIL]"},
	{22002, 750, "[RAIL TO RAIL TRANSFER]"},
	{22003, 1000, "[HUGE RAIL]"},
	{22004, 2500, "[LONG ASS RAIL]"},
	{22005, 250, "[FUNBOX TO RAIL TRANSFER]"},
	{22006, 500, "[FUNBOX TO TABLE TRANSFER]"},
	{22007, 50, "[GIMME GAP]"},
	{22008, 500, "[HANDICAP RAMP RAIL]"},
	{22300, 1000, "[COFFEE GRIND]"},
	{22301, 500, "[FOR THE WHOLE ATRIUM]"},
	{22302, 500, "[RAIL COMBO]"},
	{22601, 1500, "[NEVERSOFT ELEC CO GAP]"},
	{23000, 2000, "[DOWN THE SPIRAL]"},
	{23001, 250, "[LOMBARD LEDGE]"},
	{23002, 500, "[HUBBA LEDGE]"},
	{23003, 750, "[HOOK RAIL]"},
	{23004, 500, "[RAIL 2 RAIL]"},
	{23005, 250, "[BACKWOODS LEDGE]"},
	{23006, 500, "[BENDY'S LIP]"},
	{23007, 3000, "[ARE YOU KIDDING?]"},
	{-1},
};

/* THPS2 demo: char*[20] */
char *CheatRestarts[20];

/* THPS2 demo: char*[20] */
char *HorseRestarts[20];

/* THPS2 demo: char*[40] */
char *MenuFilenamePointers[40];

/* THPS2 demo: CBody* */
CBody *BaddyList;

/* THPS2 demo: CBruce* */
CBruce *pCurrentPlayer;

/* THPS2 demo: CBody* */
CBody *EnvironmentalObjectList;

/* THPS2 demo: int */
int NumCheatRestarts[1];

/* THPS2 demo: int */
int NumHorseRestarts[1];

/* THPS2 demo: ushort* */
void *TrigFile;

/* THPS2 demo: int */
int RestartNode[1] = {
	0xffff,
};

/* THPS2 demo: short** */
void *OffsetList;

/* THPS2 demo: int */
int NumNodes[1];

/* THPS2 demo: int */
int NumTrigMenuEntries[1];

/* THPS2 demo: SCommandPoint* */
STrig *CommandPoints;

/* THPS2 demo: uint* */
void *pPSXDualBuffer;

int DAT_800d1a10[1] = {
	0xffff0003,
};

int DAT_800d1f14[1];
