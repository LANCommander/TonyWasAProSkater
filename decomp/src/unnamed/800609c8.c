/* FUN_800609c8 (0x800609c8) */
#include "names.h"

extern int GVInfo1[];
extern int GVideoRestart[];
extern char Pad[];
extern int pVertMenu;

int DAT_800d2358, DAT_800d235c;

int Mess_GetFontScale(void);
void Pack_DefragmentFully(void);
void Front_InitMemCardMenus(int, int, int);
void Front_SetLoadAction(int);
int Mem_NewX(int, int, int);
void Front_UpdateMusicLoop(void);
void FrontLoader_Continue(void);
void Pad_Update(void);
void HiRes_ClearOTag(void);
void ResetOT(void);
void VerticalMenu_setTriangles(int, int, int);
void Front_MemCardHiResDisplay(void);
void HiRes_DrawOTag(void);
int Front_MemCardUpdate(void);
void Pad_ClearTriggers(char *);
void SFX_PlayX(int, int, int);
void Mess_SetFontScale(int);
void Front_RestoreBackground(void);
void VerticalMenu_erase(int);
void DrawSync(int);
void Front_FreeBackground(void);
void Front_FreeMemCardMenus(void);
void Mem_Delete(int);

int FUN_800609c8(void) {
	int fs, i;

	fs = Mess_GetFontScale();
	Pack_DefragmentFully();
	Front_InitMemCardMenus(640, 15, 39);
	Front_SetLoadAction(1);

	GVInfo1[1] = Mem_NewX(640, 0, 1);
	GVideoRestart[0] = 0;
	i = (int)Pad;

	do {
		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		HiRes_ClearOTag();
		ResetOT();

		if (pVertMenu != 0)
			VerticalMenu_setTriangles(pVertMenu, DAT_800d2358, DAT_800d235c);

		Front_MemCardHiResDisplay();
		HiRes_DrawOTag();
	} while (Front_MemCardUpdate() == 18);

	Pad_ClearTriggers((char *)i);
	Pad_ClearTriggers((char *)i + 364);
	SFX_PlayX(145, 8192, 0);
	Mess_SetFontScale(fs);

	i = 0;

	do {
		HiRes_ClearOTag();
		ResetOT();
		Front_RestoreBackground();

		if (pVertMenu != 0)
			VerticalMenu_erase(pVertMenu);

		i++;
		HiRes_DrawOTag();
	} while (i < 2);

	DrawSync(0);
	Front_FreeBackground();

	if (pVertMenu != 0)
		Front_FreeMemCardMenus();

	Mem_Delete(GVInfo1[1]);

	switch (GVideoRestart[0]) {
		case 1:
			return 0x611;
		case 2:
			return 0x612;
		default:
			return -1;
	}
}
