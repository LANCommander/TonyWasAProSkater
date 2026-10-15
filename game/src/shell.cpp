#include "game/shell.h"
#include "game/pad.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/physics.h"

void frug(uint);
extern int DAT_800d2464;
extern int DAT_800d2468;
extern int DAT_800d246c;

/* frug__FUi (0x800782b4) */
void frug(unsigned int n) {
	CheatCheck ^= n;
	CheatCheck = (CheatCheck << 1) ^ (CheatCheck >> 31);
	CheatCheck = CheatCheck * 521;
	CheatCheck += arccos(CheatCheck);
}

/* Shell_DoCheatProcessing__Fv (0x80078304) */
/* Demo twin for the key part (THPS1 adds the cheat table); locals from its
   MAIN.SYM. Each newly pressed pad-1 button feeds its key into frug's rolling hash; when the hash
   hits a known cheat code the matching cheat is toggled. Returns whether a cheat fired. */
int Shell_DoCheatProcessing() {
	int Result = 1;

	if (Pad[0].Up.Triggered)
		frug(0x01234567);
	else if (Pad[0].Right.Triggered)
		frug(0xab432901);
	else if (Pad[0].Down.Triggered)
		frug(0x776643d1);
	else if (Pad[0].Left.Triggered)
		frug(0x93fe1682);
	else if (Pad[0].Grind.Triggered)
		frug(0x31415926);
	else if (Pad[0].Grab.Triggered)
		frug(0xdeadbeef);
	else if (Pad[0].Ollie.Triggered)
		frug(0xb87610db);
	else if (Pad[0].Flip.Triggered)
		frug(0x03185332);
	else
		return 0;

	switch (CheatCheck) {
		case 0x63e4f9c8:
			Cheat_Fire ^= 1;
			break;
		case 0x1f71e119:
			Cheat_SlowMo ^= 1;
			break;
		case 0x9c69b980:
			Cheat_BigHead ^= 1;
			break;
		case 0x7f361998:
			if (Cheat_StatsOverride == 10)
				Cheat_StatsOverride = 0;
			else
				Cheat_StatsOverride = 10;

			break;
		case 0xa7053a01:
			Cheat_LevelSelect ^= 1;
			break;
		case 0x259f7af5:
			if (Cheat_StatsOverride == 13)
				Cheat_StatsOverride = 0;
			else
				Cheat_StatsOverride = 13;

			break;
		case 0x021a16ed:
			Cheat_RestartSkip ^= 1;
			break;
		case 0x52265053:
			DAT_800d1d24 = 1;
			break;
		default:
			Result = 0;
			break;
	}

	return Result;
}

/* Shell_PauseScreen__Fv (0x800785c4) */
void Shell_PauseScreen(void) {}

/* FUN_800785cc (0x800785cc) */
void FUN_800785cc(int a) {
	DAT_800d2464 = a;
	DAT_800d2468 = 0x12b9b0a1;
	DAT_800d246c = 0x0aa2fb3f;
}
