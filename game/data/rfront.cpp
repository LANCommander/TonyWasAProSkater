/* Decompiled data: the retail executable's initialised globals that src/rfront/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/rfront.h"

struct VerticalMenu;

/* THPS2 demo: char[11] */
char SaveGameName[] = "          ";

char s_800a42dc[] = "          ";
char s_800a42e8[] = "          ";

/* THPS2 demo: char[11] */
unsigned char StoredCareerName[12];

/* THPS2 demo: char[15][11] */
unsigned char ASCIIName[168];

/* THPS2 demo: char*[15] */
char *VertMenuItem[15];

/* THPS2 demo: int[15] */
int VertMenuItemIndex[15];

/* THPS2 demo: SCachedBackground[10] */
SCachedBackground BGCache[10];

/* THPS2 demo: null */
int VideoToDisk[1];

/* THPS2 demo: int */
int CursorPos[1];

/* THPS2 demo: VerticalMenu* */
VerticalMenu *pVertMenu;

/* THPS2 demo: int */
int NumVertMenuItems[1];

/* THPS2 demo: int */
int VertMenuTopIndex[1];

/* THPS2 demo: int */
int VertMenuTopChanged[1];

/* THPS2 demo: int */
int FreeSprite[1];

/* EMemCardState (THPS2 demo: EMemCardState) */
int MCState[1];

/* EMemCardState (THPS2 demo: EMemCardState) */
int OldMCState[1];

/* THPS2 demo: int */
int CheckingCardPause[1];

/* THPS2 demo: int */
int MCResolution[1] = {
	512,
};

int DAT_800d1138[1];

/* ELoadAction (THPS2 demo: ELoadAction) */
int GLoadAction[1];

/* THPS2 demo: int */
int LeftRightAutoRepeatTimer[1];
