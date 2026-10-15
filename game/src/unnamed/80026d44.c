/* FUN_80026d44 (0x80026d44) */
#include "names.h"
#include "game/padbuttons.h"

extern volatile unsigned char Pad[];

extern int GNumberOfPlayers;

void FUN_80092044(void);
void PC_RunStaticCtors(void);
void Options_Init(void);
void Career_Init(void);
void FUN_80071f84(int);
void Init_Restart(int, int);
void Db_Init(void);
void Pad_Update(void);
void GameFMV_PlayMovie(int);
void HiRes_Init(int);
void FUN_8006b3f8(void);

int FUN_80026d44(void) {
	volatile unsigned char *p;

	PC_RunStaticCtors(); /* the game's static constructors (libsn's __main on the PS1) */
	Options_Init();
	Career_Init();
	FUN_80071f84(1);
	Init_Restart(0, 16);
	Db_Init();
	Pad_Update();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 &&
	    (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-1);

	Pad_Update();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 &&
	    (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-2);

	Pad_Update();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 &&
	    (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-25);

	Init_Restart(0, 16);
	Db_Init();

	GNumberOfPlayers = 1;
	Init_Restart(1, 16);
	HiRes_Init(1);
	FUN_8006b3f8();
	return 0;
}
