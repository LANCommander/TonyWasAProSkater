/* FUN_8006e454 (0x8006e454) */
/* C++. Counts a skater event (+0xe00) in career mode: the 5th ends it (99, Panel_Tape__Fi(1));
   before that it shows "n of N" for the current level. */
struct SMessageProg;

struct SLevel {
	char pad0[0x38];
	int Goal; /* 0x38 */
	char pad3c[120 - 0x3c];
};

extern SLevel Levels[];
extern char s_d_OF_5_s[];
extern SMessageProg Messprog_Zoom;
extern char Msg[];
extern int GGame;
extern char *MechList;
extern int GLevel;

void Mess_Message(const char *, SMessageProg *, bool, short, short);

extern "C" {
	void Panel_Tape__Fi(int);
	int FUN_8008b900(char *, const char *, ...);
}

extern "C" void FUN_8006e454(void) {
	if (GGame != 1)
		return;
	char *pSkater = MechList;
	int n = ++*(int *)(pSkater + 0xe00);

	if (n == 5) {
		*(int *)(pSkater + 0xe00) = 99;
		Panel_Tape__Fi(1);
		return;
	}

	FUN_8008b900(Msg, s_d_OF_5_s, n, Levels[GLevel].Goal);
	Mess_Message(Msg, &Messprog_Zoom, 1, 0, 0);
}
