/* Panel_Tape__Fi (0x8006e2fc) */
#include "names.h"

typedef struct {
	char p[0x38];
	int f38;
	char q[0x78 - 0x3c];
} E;

extern E Levels[];
extern char *GameText_20[], *GameText_24[], *GameText_28[], *GameText_30[];
extern int DAT_800a5d88[];
extern char Messprog_Goal[];
extern char DAT_800d3868[];
extern int GLevel;

int TAPES_Display;

void Career_GiveTape(int, int);
int SFX_PlayX(int, int, int);
void FUN_8008b900(char *, int, int);
void Mess_Message(char *, char *, int, int, int);

void Panel_Tape(int a) {
	TAPES_Display = 40;
	Career_GiveTape(GLevel, a);

	switch (a) {
		case 2:
			Mess_Message(GameText_20[0], Messprog_Goal, 1, 0, 0);
			break;
		case 0:
			SFX_PlayX(19, 0x2000, 0);
			Mess_Message(GameText_24[0], Messprog_Goal, 1, 0, 0);
			break;
		case 4:
			SFX_PlayX(19, 0x2000, 0);
			Mess_Message(GameText_28[0], Messprog_Goal, 1, 0, 0);
			break;
		case 1:
			SFX_PlayX(19, 0x2000, 0);
			FUN_8008b900(DAT_800d3868, DAT_800a5d88[0], Levels[GLevel].f38);
			Mess_Message(DAT_800d3868, Messprog_Goal, 1, 0, 0);
			break;
		case 3:
			SFX_PlayX(159, 0x2000, 0);
			Mess_Message(GameText_30[0], Messprog_Goal, 1, 0, 0);
			break;
	}
}
