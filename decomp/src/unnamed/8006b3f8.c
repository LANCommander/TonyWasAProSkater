/* FUN_8006b3f8 (0x8006b3f8) */
#include "names.h"
extern char s_shell_vab_800a2320[];

void FUN_800171f0(void);
void ResetMainDemoTimer(void);
void FUN_8004b09c(char *p);
void FUN_8005bb70(void);

void FUN_8006b3f8(void) {
	volatile int pad[2];

	FUN_800171f0();
	ResetMainDemoTimer();
	FUN_8004b09c(s_shell_vab_800a2320);
	FUN_8005bb70();
}
