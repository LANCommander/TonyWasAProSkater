/* FUN_8005d28c (0x8005d28c) */
#include "names.h"
#include "game/padbuttons.h"

extern char Pad[];

char *DAT_800d1d0c;
int DAT_800d1cec;
int DAT_800d1d14;

void FrontLoader_SetFiles(int a);
void FrontLoader_LoadSet(int a);
void FrontLoader_Defragment(void);
int FrontLoader_TakeFile(char *a);
void FUN_80061a10(int a);
void FUN_8005dc58(int a);
char *__nw__6CClassUi(unsigned);
char *FUN_80063a5c(char *p, int a, int b, int c, int d, int e, int f);
void Pack_DefragmentFully(void);
void HiRes_ClearOTag(void);
void ResetOT(void);

void FUN_8005d28c(void) {
	char *p;

	FrontLoader_SetFiles(8);
	FrontLoader_LoadSet(8);
	FrontLoader_Defragment();
	FUN_80061a10(FrontLoader_TakeFile("loadingv.bmp"));
	FUN_8005dc58(6);

	p = __nw__6CClassUi(24);

	if (p) {
		p = FUN_80063a5c(p, 8, FrontLoader_TakeFile("Bckgrnd2.bmp"), 0, 458, 64, 128);
	}

	DAT_800d1d0c = p;
	Pad[PAD_MENU_START + PAD_TRIGGERED] = 0;
	Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] = 0;
	DAT_800d1cec = 0;
	DAT_800d1d14 = 0;
	Pack_DefragmentFully();
	HiRes_ClearOTag();
	ResetOT();
}
