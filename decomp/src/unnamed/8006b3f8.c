/* FUN_8006b3f8 (0x8006b3f8) */
#include "names.h"

void FUN_800171f0(void);
void ResetMainDemoTimer(void);
void FUN_8004b09c(char *p);
void FUN_8005bb70(void);

void FUN_8006b3f8(void) {
	volatile int pad[2];

	FUN_800171f0();
	ResetMainDemoTimer();
	FUN_8004b09c("shell.vab");
	FUN_8005bb70();
}
