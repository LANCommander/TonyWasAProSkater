/* Front-end helpers of RFRONT.CPP: memory-card menus, backgrounds, entering a name. */
#ifndef GAME_RFRONT_H
#define GAME_RFRONT_H

#include "game/types.h"

class Sprite;

/* a piece of the background saved under text drawn over it (BGCache[10], FreeSprite in use;
   Front_DirtyRectText, Front_RestoreBackground, Front_FreeBackground) */
struct SCachedBackground {
	Sprite *pSprite;     /* 0x00: the background under the text */
	const char *pString; /* 0x04: the text */
	short x, y;          /* 0x08 */
	int pad;             /* 0x0c */
	int State;           /* 0x10: 2 = holds a background; 0 = freed */
	                     /* sizeof == 0x14 */
};

/* functions */
/* what the memory-card screen loads (GLoadAction; rfront/memcardupdate state 21) */
enum ELoadAction { LOAD_REPLAY, LOAD_REPLAY_HEADER, LOAD_GAME };

void Front_FreeMemCardMenus();                /* 0x80011150 */
void RGBGreen();                              /* 0x80011214 */
void Front_FreeBackground();                  /* 0x80011570 */
void Front_SetLoadAction(ELoadAction Action); /* 0x80011628 */
void InitEnterName();                         /* 0x800117b4 */
int NameWasBlank();                           /* 0x80012600 */
void FillInDefaultName();                     /* 0x80012650 */

#endif
