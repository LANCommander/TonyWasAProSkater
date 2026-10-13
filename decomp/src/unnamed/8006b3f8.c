/* FUN_8006b3f8 (0x8006b3f8) */
#include "names.h"

void Front_Init(void);
void ResetMainDemoTimer(void);
void SFX_Init(char *p);
void Front_Main(void);

void FUN_8006b3f8(void) {
	volatile int pad[2];

	Front_Init();
	ResetMainDemoTimer();
	SFX_Init("shell.vab");
	Front_Main();
}
