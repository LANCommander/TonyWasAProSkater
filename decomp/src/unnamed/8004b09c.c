/* SFX_Init__FPc (0x8004b09c) */
#include "names.h"
#ifdef PC_PORT
#include "pc_sound.h"
#endif

extern char DAT_800b9014[];

char pSFXVABHeader[8];
int DAT_800d1a94;

void FUN_80093854(int a, char *b);
void SFX_SetReverbType(unsigned char a);
int FUN_8004b188(int a, char *b);

void SFX_Init(int a) {
#ifdef PC_PORT
	PC_SoundBanksReset();
#else
	FUN_80093854(16, DAT_800b9014);
#endif
	SFX_SetReverbType(0);

	DAT_800d1a94 = FUN_8004b188(a, pSFXVABHeader);
}
