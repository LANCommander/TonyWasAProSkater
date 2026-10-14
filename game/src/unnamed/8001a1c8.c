/* Front_SwitchOnEndHeat__Fv (0x8001a1c8) */
#include "names.h"

extern int GVInfo1[];
extern int GVInfo2[];
extern char *GameText_1b8[];
extern char Pad[];
extern char *MechList;
extern int DAT_800d1e48;

int DAT_800d1524;
int DAT_800d1528;
int DAT_800d152c;
int Status;
int GamePaused;
char *pYesNoMenu;

void Video_Stop(char *b);
void Pad_ClearTriggers(char *p);
int CMenu_FindEntry(char *m, char *s);
void CMenu_SetLine(char *m, unsigned char l);
void Mess_DeleteAll(void);
void Pad_ActuatorOff(unsigned char a, unsigned char b);
void SFX_Pause(void);
void SFX_PlayX(int a, int b, int c);

void Front_SwitchOnEndHeat(void) {
	char *s;

	DAT_800d1524 = 0;
	DAT_800d1528 = 0;
	DAT_800d152c = 0;
	s = *(char **)(MechList + 28);

	if (GVInfo1[0])
		Video_Stop(MechList);

	if (s && GVInfo2[0])
		Video_Stop(s);

	Status = 19;
	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + 364);

	GamePaused = 1;
	CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText_1b8[0]));
	Mess_DeleteAll();
	Pad_ActuatorOff(0, 0);
	Pad_ActuatorOff(0, 1);
	SFX_Pause();
	SFX_PlayX(2, 4096, 0);

	DAT_800d1e48 = 0;
}
