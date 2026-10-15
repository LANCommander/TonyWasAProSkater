/* Front_SwitchOnEndHeat__Fv (0x8001a1c8) */
#include "game/padbuttons.h"
#include "game/classsizes.h"
#include "names.h"

extern int GVInfo1[]; /* SVideoInfo[2] */
extern char *GameText[];
extern char Pad[];
/* the fields of a CItem (game/ob.h) up to the one this reads */
struct Item {
	unsigned short Flags, Inquiry;
	int Pos[3];
	short Angles[3];
	unsigned short Model;
	unsigned char DummyFrame, TintIndex, DummyAnim, Region;
	struct Item *NextItem; /* 0x1c */
};

extern struct Item *MechList;
extern int DAT_800d1e48;

extern int DAT_800d1524;
extern int DAT_800d1528;
extern int DAT_800d152c;
extern int Status;
extern int GamePaused;
extern char *pYesNoMenu;

void Video_Stop(struct Item *b);
void Pad_ClearTriggers(char *p);
int CMenu_FindEntry(char *m, char *s);
void CMenu_SetLine(char *m, unsigned char l);
void Mess_DeleteAll(void);
void Pad_ActuatorOff(unsigned char a, unsigned char b);
void SFX_Pause(void);
void SFX_PlayX(int a, int b, int c);

void Front_SwitchOnEndHeat(void) {
	struct Item *s;

	DAT_800d1524 = 0;
	DAT_800d1528 = 0;
	DAT_800d152c = 0;
	s = MechList->NextItem;

	if (GVInfo1[0])
		Video_Stop(MechList);

	if (s && GVInfo1[SIZEOF_SVIDEOINFO / 4])
		Video_Stop(s);

	Status = 19;
	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + PAD_SIZE);

	GamePaused = 1;
	CMenu_SetLine(pYesNoMenu, CMenu_FindEntry(pYesNoMenu, GameText[110]));
	Mess_DeleteAll();
	Pad_ActuatorOff(0, 0);
	Pad_ActuatorOff(0, 1);
	SFX_Pause();
	SFX_PlayX(2, 4096, 0);

	DAT_800d1e48 = 0;
}
