/* FUN_80026d44 (0x80026d44) */
#include "names.h"
#include "game/padbuttons.h"
extern volatile unsigned char Pad[];

int GNumberOfPlayers;

void FUN_80092044(void);
void Options_Init(void);
void Career_Init(void);
void FUN_80071f84(int);
void Init_Restart(int, int);
void FUN_800491e8(void);
void FUN_80070ff4(void);
void GameFMV_PlayMovie(int);
void FUN_8006b01c(int);
void FUN_8006b3f8(void);

int FUN_80026d44(void) {
	volatile unsigned char *p;

	FUN_80092044();
	Options_Init();
	Career_Init();
	FUN_80071f84(1);
	Init_Restart(0, 16);
	FUN_800491e8();
	FUN_80070ff4();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 && (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-1);

	FUN_80070ff4();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 && (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-2);

	FUN_80070ff4();

	p = Pad;

	if (p[PAD_PAUSE + PAD_PRESSED] == 0 && (p[PAD_SPIN_LEFT + PAD_PRESSED] == 0 || p[PAD_TAP_SPIN_LEFT + PAD_PRESSED] == 0))
		GameFMV_PlayMovie(-25);

	Init_Restart(0, 16);
	FUN_800491e8();

	GNumberOfPlayers = 1;
	Init_Restart(1, 16);
	FUN_8006b01c(1);
	FUN_8006b3f8();
	return 0;
}
