/* FrontLoader_SetFiles__Fi (0x8005c8dc) */
#include "game/frontend.h"
#include "game/shellutils.h"

extern SScreenFiles DAT_800be478[];
extern FileLoader DAT_800bec34;

extern int DAT_800d1cac;

void FrontLoader_SetFiles(int a) {
	DAT_800d1cac = a;
	FrontLoader_CheckSet();
	DAT_800bec34.setFiles(DAT_800be478[a].pName, DAT_800be478[a].Num);
}
