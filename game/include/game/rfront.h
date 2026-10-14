/* Front-end helpers of RFRONT.CPP: memory-card menus, backgrounds, entering a name. */
#ifndef GAME_RFRONT_H
#define GAME_RFRONT_H

#include "game/types.h"

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
