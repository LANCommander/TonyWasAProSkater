/* Decompiled data: the retail executable's initialised globals that src/rfront/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/rfront.h"

struct VerticalMenu;

/* THPS2 demo: char[11] */
PSX_DATA(0x800a42d0) char SaveGameName[] = "          ";

PSX_DATA(0x800a42dc) char s_800a42dc[] = "          ";
PSX_DATA(0x800a42e8) char s_800a42e8[] = "          ";

/* THPS2 demo: char[11] */
PSX_DATA(0x800a4308) unsigned char StoredCareerName[12];

/* THPS2 demo: char[15][11] */
PSX_DATA(0x800a4314) unsigned char ASCIIName[168];

/* THPS2 demo: char*[15] */
PSX_DATA(0x800a43bc) char *VertMenuItem[15];

/* THPS2 demo: int[15] */
PSX_DATA(0x800a43f8) int VertMenuItemIndex[15];

/* THPS2 demo: SCachedBackground[10] */
PSX_DATA(0x800a4434) SCachedBackground BGCache[10];

/* THPS2 demo: null */
PSX_DATA(0x800d10f0) int VideoToDisk[1];

/* THPS2 demo: int */
PSX_DATA(0x800d10fc) int CursorPos[1];

/* THPS2 demo: VerticalMenu* */
PSX_DATA(0x800d1104) VerticalMenu *pVertMenu;

/* THPS2 demo: int */
PSX_DATA(0x800d1108) int NumVertMenuItems[1];

/* THPS2 demo: int */
PSX_DATA(0x800d110c) int VertMenuTopIndex[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1110) int VertMenuTopChanged[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1118) int FreeSprite[1];

/* EMemCardState (THPS2 demo: EMemCardState) */
PSX_DATA(0x800d111c) int MCState[1];

/* EMemCardState (THPS2 demo: EMemCardState) */
PSX_DATA(0x800d1120) int OldMCState[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1124) int CheckingCardPause[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1128) int MCResolution[1] = {
	512,
};

PSX_DATA(0x800d1138) int DAT_800d1138[1];

/* ELoadAction (THPS2 demo: ELoadAction) */
PSX_DATA(0x800d1144) int GLoadAction[1];

/* THPS2 demo: int */
PSX_DATA(0x800d141c) int LeftRightAutoRepeatTimer[1];
