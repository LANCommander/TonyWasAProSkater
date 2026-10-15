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
void FlickerSync();
void Front_Main();
void Front_UpdateMusicLoop();
void FUN_8005dc8c();
void FlickerSync();
void ClearSoftReset();
void LoadingScreen_Main();
void LoadingScreen_SetCurrentLevel(int a, int b, int c);
void LoadingScreen_Wait();
void LoadingScreen_WaitLevel();

void Front_InitBoardPictures2P();
void Front_StreamBoardPictures2P();
void Front_FreeBoardPictures2P();
int Front_BoardSelect2P(int player);
void Front_InitBoardPictures1P();
void Front_StreamBoardPictures1P();
void Front_FreeBoardPictures1P();
int Front_BoardSelect1P(int player);
void Front_InitSkaterSpool2P();
void Front_SpoolNextSkater2P();
void Front_FreeSkaterSpool2P();
CBruce *Front_CreateSkaterModel2P(int Player);
void Front_ResetDrawArea2P();
void Front_UnloadSkaterTextures2P();
void Front_LoadSkaterTextures2P();
int Front_SkaterSelect2P();
void Front_InitSkaterSpool1P();
void Front_SpoolNextSkater1P();
void Front_FreeSkaterSpool1P();
CBruce *Front_CreateSkaterModel1P();
void Front_ResetDrawArea1P();
void Front_InitSkaterPack(int m);
void Front_SelectSkaterPack(int a);
void Front_ResetSkaterPack();
void Front_FreeSkaterPack();
int Front_SkaterSelect1P();
void Front_CalcSkaterPreviewBounds();
void Front_CheckSoftReset();
void Front_SetMusicLoop(int a);
void Front_StopMusicLoop();
void FrontLoader_Reset();
void FrontLoader_CheckSet();
void FrontLoader_SetFiles(int a);
void FrontLoader_Continue();
void FrontLoader_LoadSet(int a);
void FrontLoader_UnloadAll();
char *FrontLoader_TakeFile(char *name);
void FrontLoader_Defragment();
void Front_LoadSpritesPRE();
void Front_FreeSpritesPRE();
int Front_MainMenu(int a);
int Front_RunMenu(char **items, int *disabled, int num, int base, int *path,
                  void (*handler)(VerticalMenu *), int *xlist, int *ylist, int selection);
char *Front_GetPadFunctionName(int i);
void Front_StorePadMapping(int i);
void Front_ControllerOptionsHandler(VerticalMenu *menu);
int Front_ControllerOptions(int a);
void Front_SoundOptionsHandler(VerticalMenu *a);
int Front_MemCardScreen();
void Front_TitleScreen();

#endif
