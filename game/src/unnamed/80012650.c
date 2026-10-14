/* FillInDefaultName__Fv (0x80012650) */
#include "names.h"

int GSaveAction;
int DAT_800d113c;
int DAT_800d1140;

extern char SaveGameName[];

void FillInDefaultName(void) {
	if (!GSaveAction) {
		SaveGameName[0] = 'V';
		SaveGameName[1] = 'I';
		SaveGameName[2] = 'D';
		SaveGameName[3] = 'E';
		SaveGameName[4] = 'O';
		SaveGameName[5] = ' ';
		SaveGameName[6] = DAT_800d113c + 65;
		DAT_800d113c = (DAT_800d113c + 1) % 26;
	} else {
		SaveGameName[0] = 'C';
		SaveGameName[1] = 'A';
		SaveGameName[2] = 'R';
		SaveGameName[3] = 'E';
		SaveGameName[4] = 'E';
		SaveGameName[5] = 'R';
		SaveGameName[6] = ' ';
		SaveGameName[7] = DAT_800d1140 + 65;
		DAT_800d1140 = (DAT_800d1140 + 1) % 26;
	}
}
