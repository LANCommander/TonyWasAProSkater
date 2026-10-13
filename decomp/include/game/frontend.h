/* THPS1's front-end screens (0x80050c5c-0x80061714; the demo has no counterpart): board and
   skater select (1P/2P), the front-end music loop, the FileLoader wrappers (FrontLoader_*: the
   global FileLoader, game/shellutils.h), the generic vertical-menu loop and the options/memory-card
   screens. Names are descriptive (decomp/inferred_names.tsv). */
#ifndef GAME_FRONTEND_H
#define GAME_FRONTEND_H

#include "game/types.h"

/* functions */
class CBruce;
class VerticalMenu;

void Front_InitBoardPictures2P();              /* 0x80050c5c */
void Front_StreamBoardPictures2P();            /* 0x8005115c */
void Front_FreeBoardPictures2P();              /* 0x800514f0 */
int Front_BoardSelect2P(int skater);           /* 0x80051660 */
void Front_InitBoardPictures1P();              /* 0x80053dbc */
void Front_StreamBoardPictures1P();            /* 0x80054134 */
void Front_FreeBoardPictures1P();              /* 0x800543d4 */
int Front_BoardSelect1P(int skater);           /* 0x80054510 */
void Front_InitSkaterSpool2P();                /* 0x80055eb8 */
void Front_SpoolNextSkater2P();                /* 0x80055f80 */
void Front_FreeSkaterSpool2P();                /* 0x8005609c */
CBruce *Front_CreateSkaterModel2P(int player); /* 0x8005622c */
void Front_ResetDrawArea2P();                  /* 0x80056394 */
void Front_UnloadSkaterTextures2P();           /* 0x80056428 */
void Front_LoadSkaterTextures2P();             /* 0x800565ac */
int Front_SkaterSelect2P();                    /* 0x80056754 */
void Front_InitSkaterSpool1P();                /* 0x80059050 */
void Front_SpoolNextSkater1P();                /* 0x80059094 */
void Front_FreeSkaterSpool1P();                /* 0x800591c0 */
CBruce *Front_CreateSkaterModel1P();           /* 0x8005933c */
void Front_ResetDrawArea1P();                  /* 0x8005948c */
void Front_InitSkaterPack(int players);        /* 0x80059520 */
void Front_SelectSkaterPack(int which);        /* 0x80059618 */
void Front_ResetSkaterPack();                  /* 0x800596a0 */
void Front_FreeSkaterPack();                   /* 0x80059758 */
int Front_SkaterSelect1P();                    /* 0x80059794 */
void Front_CalcSkaterPreviewBounds();          /* 0x8005bad0 */
void Front_Main();                             /* 0x8005bb70 */
void Front_CheckSoftReset();                   /* 0x8005c68c */
void Front_SetMusicLoop(int sound);            /* 0x8005c734 */
void Front_UpdateMusicLoop();                  /* 0x8005c780 */
void Front_StopMusicLoop();                    /* 0x8005c85c */
void FrontLoader_Reset();                      /* 0x8005c8ac */
void FrontLoader_CheckSet();                   /* 0x8005c8bc */
void FrontLoader_SetFiles(int set);            /* 0x8005c8dc */
void FrontLoader_Continue();                   /* 0x8005c934 */
void FrontLoader_LoadSet(int set);             /* 0x8005c97c */
void FrontLoader_UnloadAll();                  /* 0x8005ca10 */
char *FrontLoader_TakeFile(char *name);        /* 0x8005ca3c */
void FrontLoader_Defragment();                 /* 0x8005ca64 */
void Front_LoadSpritesPRE();                   /* 0x8005ca88 */
void Front_FreeSpritesPRE();                   /* 0x8005cae8 */
int Front_MainMenu(int path);                  /* 0x8005de7c */
int Front_RunMenu(char **items, int *disabled, int num, int base, int *path,
                  void (*handler)(VerticalMenu *), int *xlist, int *ylist,
                  int selection);                        /* 0x8005e0e0 */
char *Front_GetPadFunctionName(int function);            /* 0x8005ea74 */
void Front_StorePadMapping(int player);                  /* 0x8005eb28 */
void Front_ControllerOptionsHandler(VerticalMenu *menu); /* 0x8005eb78 */
int Front_ControllerOptions(int path);                   /* 0x8005f6bc */
void Front_SoundOptionsHandler(VerticalMenu *menu);      /* 0x8005f9ec */
int Front_MemCardScreen();                               /* 0x80060ba0 */
void Front_TitleScreen();                                /* 0x80060e98 */

#endif
