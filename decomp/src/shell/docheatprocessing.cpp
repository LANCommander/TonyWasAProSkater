/* Shell_DoCheatProcessing__Fv (0x80078304) */
/* C++ (SHELL.CPP). Demo twin for the key part (THPS1 adds the cheat table); locals from its
   MAIN.SYM. Each newly pressed pad-1 button feeds its key into frug's rolling hash; when the hash
   hits a known cheat code the matching cheat is toggled. Returns whether a cheat fired. */
typedef unsigned char uchar;
typedef unsigned int uint;

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle, Square, Circle, X;
	SButton LeftOne, LeftTwo, RightOne, RightTwo;
	SButton Left, Right, Up, Down;
	char pad[364 - 192];
};

extern SControl Pad[2];

uint CheatCheck;

extern int DAT_800d1d24;
extern int Cheat_SlowMo;
extern int Cheat_RestartSkip;
extern int Cheat_StatsOverride;
extern int Cheat_Fire;
extern int Cheat_BigHead;
extern int Cheat_LevelSelect;

void frug(uint);

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
	else if (Pad[0].Triangle.Triggered)
		frug(0x31415926);
	else if (Pad[0].Circle.Triggered)
		frug(0xdeadbeef);
	else if (Pad[0].X.Triggered)
		frug(0xb87610db);
	else if (Pad[0].Square.Triggered)
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
