/* CommonSwitchOnMenuCode__Fv (0x8001a128) */
#include "game/padbuttons.h"
#include "names.h"

extern char Pad[];

extern void *pYesNoMenu;

extern char *GameText[];

extern int GamePaused;

extern int DAT_800d1e48;

extern int Front_OnTime;

void Pad_ClearTriggers(char *p);
int CMenu_FindEntry(void *m, const char *c);
void CMenu_SetLine(void *m, unsigned char c);
void Mess_DeleteAll(void);
void Pad_ActuatorOff(unsigned char a, unsigned char b);
void SFX_Pause(void);
void SFX_PlayX(int a, int b, int c);

void CommonSwitchOnMenuCode(void) {
	char *s = Pad;

	Pad_ClearTriggers(s);
	Pad_ClearTriggers(s + PAD_SIZE);

	GamePaused = 1;
	CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText[110]));
	Mess_DeleteAll();
	Pad_ActuatorOff(0, 0);
	Pad_ActuatorOff(0, 1);
	SFX_Pause();
	SFX_PlayX(2, 4096, 0);

	DAT_800d1e48 = 0;
	Front_OnTime = 0;
}
