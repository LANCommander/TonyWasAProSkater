/* FUN_8004b09c (0x8004b09c) */
#include "names.h"

extern char DAT_800b9014[];

char pSFXVABHeader[8];
int DAT_800d1a94;

void FUN_80093854(int a, char *b);
void SFX_SetReverbType(unsigned char a);
int FUN_8004b188(int a, char *b);

void FUN_8004b09c(int a) {
	FUN_80093854(16, DAT_800b9014);
	SFX_SetReverbType(0);

	DAT_800d1a94 = FUN_8004b188(a, pSFXVABHeader);
}
