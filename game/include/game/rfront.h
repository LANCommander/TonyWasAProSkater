/* Front-end helpers of RFRONT.CPP: memory-card menus, backgrounds, entering a name. */
#ifndef GAME_RFRONT_H
#define GAME_RFRONT_H

#include "game/front.h"
#include "game/types.h"
class VerticalMenu;

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
int GetUpDown();
int Front_TypeName(char *Name, int Len, int *pCursor); /* PC: keyboard typing on the name screens */
void Front_RestoreBackground();
void Front_InitMemCardMenus(int a, EMemCardState b, EPausedStatus c);
void Front_SetSaveAction(ESaveAction v);
void Front_ExtractASCIINameFromSJIS(char *s, char *d);
void Front_MemCardDisplay();
void Front_MemCardHiResDisplay();
int Front_MemCardUpdate();
void Front_ClearGVideoRestartData();
/* what the memory-card screen loads (GLoadAction; rfront/memcardupdate state 21) */
enum ELoadAction { LOAD_REPLAY, LOAD_REPLAY_HEADER, LOAD_GAME };
void Front_SetSJISName(char *SJISName);

void Front_FreeMemCardMenus();
void RGBGreen();
void Front_FreeBackground();
void Front_SetLoadAction(ELoadAction v);
void InitEnterName();
int NameWasBlank();
void FillInDefaultName();

/* globals */
extern VerticalMenu *pVertMenu;
extern char SaveGameName[];
extern char StoredCareerName[];
#endif
