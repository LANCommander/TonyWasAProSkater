/* SFX_Init__FPc (0x8004b09c) */
#include "names.h"
#include "pc_sound.h"

extern char DAT_800b9014[];

extern char *pSFXVABHeader;
extern int DAT_800d1a94;

void FUN_80093854(int a, char *b);
void SFX_SetReverbType(unsigned char a);
int FUN_8004b188(char *a, char **b);

void SFX_Init(char *a) {
	PC_SoundBanksReset();
	SFX_SetReverbType(0);

	DAT_800d1a94 = FUN_8004b188(a, &pSFXVABHeader);
}
