/* FUN_8006e454 (0x8006e454) */
/* C++. Counts a skater event (+0xe00) in career mode: the 5th ends it (99, Panel_Tape__Fi(1));
   before that it shows "n of N" for the current level. */
#include "game/bruce.h"
#include "game/career.h"

struct SMessageProg;

extern SLevel Levels[];
extern SMessageProg Messprog_Zoom;
extern char Msg[];
extern int GGame;
extern CBruce *MechList;
extern int GLevel;

void Mess_Message(const char *, SMessageProg *, bool, short, short);

extern "C" {
	void Panel_Tape__Fi(int);
	int FUN_8008b900(char *, const char *, ...);
}

extern "C" void FUN_8006e454(void) {
	if (GGame != 1)
		return;
	CBruce *pSkater = MechList;
	int n = ++pSkater->mPanel.Goals;

	if (n == 5) {
		pSkater->mPanel.Goals = 99;
		Panel_Tape__Fi(1);
		return;
	}

	FUN_8008b900(Msg, "%d OF 5 %s", n, Levels[GLevel].pGoalName);
	Mess_Message(Msg, &Messprog_Zoom, 1, 0, 0);
}
