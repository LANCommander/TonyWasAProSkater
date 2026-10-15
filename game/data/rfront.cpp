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
char StoredCareerName[11];

/* THPS2 demo: char[15][11] */
char ASCIIName[15][11];

/* THPS2 demo: char*[15] */
char *VertMenuItem[15];

/* THPS2 demo: int[15] */
int VertMenuItemIndex[15];

/* THPS2 demo: SCachedBackground[10] */
SCachedBackground BGCache[10];

/* THPS2 demo: null */
int VideoToDisk;

/* THPS2 demo: int */
int CursorPos;

/* THPS2 demo: VerticalMenu* */
VerticalMenu *pVertMenu;

/* THPS2 demo: int */
int NumVertMenuItems;

/* THPS2 demo: int */
int VertMenuTopIndex;

/* THPS2 demo: int */
int VertMenuTopChanged;

/* THPS2 demo: int */
int FreeSprite;

/* EMemCardState (THPS2 demo: EMemCardState) */
EMemCardState MCState;

/* EMemCardState (THPS2 demo: EMemCardState) */
EMemCardState OldMCState;

/* THPS2 demo: int */
int CheckingCardPause;

/* THPS2 demo: int */
int MCResolution = 512;

int DAT_800d1138[1];

/* ELoadAction (THPS2 demo: ELoadAction) */
ELoadAction GLoadAction;

/* THPS2 demo: int */
int LeftRightAutoRepeatTimer;
