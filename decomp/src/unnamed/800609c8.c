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
void FUN_80011628(int);
int Mem_NewX(int, int, int);
void FUN_8005c780(void);
void FUN_8005c934(void);
void FUN_80070ff4(void);
void FUN_8006b240(void);
void FUN_80061ea0(void);
void FUN_80064890(int, int, int);
void Front_MemCardHiResDisplay(void);
void FUN_8006b26c(void);
int Front_MemCardUpdate(void);
void Pad_ClearTriggers(char *);
void SFX_PlayX(int, int, int);
void Mess_SetFontScale(int);
void Front_RestoreBackground(void);
void VerticalMenu_erase(int);
void FUN_8008c468(int);
void FUN_80011570(void);
void FUN_80011150(void);
void Mem_Delete(int);

int FUN_800609c8(void) {
	int fs, i;

	fs = Mess_GetFontScale();
	Pack_DefragmentFully();
	Front_InitMemCardMenus(640, 15, 39);
	FUN_80011628(1);

	GVInfo1[1] = Mem_NewX(640, 0, 1);
	GVideoRestart[0] = 0;
	i = (int)Pad;

	do {
		FUN_8005c780();
		FUN_8005c934();
		FUN_80070ff4();
		FUN_8006b240();
		FUN_80061ea0();

		if (pVertMenu != 0)
			FUN_80064890(pVertMenu, DAT_800d2358, DAT_800d235c);

		Front_MemCardHiResDisplay();
		FUN_8006b26c();
	} while (Front_MemCardUpdate() == 18);

	Pad_ClearTriggers((char *)i);
	Pad_ClearTriggers((char *)i + 364);
	SFX_PlayX(145, 8192, 0);
	Mess_SetFontScale(fs);

	i = 0;

	do {
		FUN_8006b240();
		FUN_80061ea0();
		Front_RestoreBackground();

		if (pVertMenu != 0)
			VerticalMenu_erase(pVertMenu);

		i++;
		FUN_8006b26c();
	} while (i < 2);

	FUN_8008c468(0);
	FUN_80011570();

	if (pVertMenu != 0)
		FUN_80011150();

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
