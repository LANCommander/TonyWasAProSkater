/* Panel_Tape__Fi (0x8006e2fc) */
#include "game/career.h"
#include "game/front.h"
#include "game/psyq.h"
#include "game/sfx.h"

struct SMessageProg;

extern SLevel Levels[];
extern LocalText GameText;
extern SMessageProg Messprog_Goal;
extern char DAT_800d3868[];
extern int GLevel;

extern int TAPES_Display;

void Career_GiveTape(int, int);
void Mess_Message(const char *, SMessageProg *, bool, short, short);

void Panel_Tape(int a) {
	TAPES_Display = 40;
	Career_GiveTape(GLevel, a);

	switch (a) {
		case 2:
			Mess_Message(GameText.pSKATETape, &Messprog_Goal, 1, 0, 0);
			break;
		case 0:
			SFX_PlayX(19, 0x2000, 0);
			Mess_Message(GameText.p20000Tape, &Messprog_Goal, 1, 0, 0);
			break;
		case 4:
			SFX_PlayX(19, 0x2000, 0);
			Mess_Message(GameText.p40000Tape, &Messprog_Goal, 1, 0, 0);
			break;
		case 1:
			SFX_PlayX(19, 0x2000, 0);
			sprintf(DAT_800d3868, GameText.p5GoalsTape, Levels[GLevel].pGoalName);
			Mess_Message(DAT_800d3868, &Messprog_Goal, 1, 0, 0);
			break;
		case 3:
			SFX_PlayX(159, 0x2000, 0);
			Mess_Message(GameText.pHiddenTape, &Messprog_Goal, 1, 0, 0);
			break;
	}
}
