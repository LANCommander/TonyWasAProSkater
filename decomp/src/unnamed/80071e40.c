/* Init_ResetMemory__Fi (0x80071e40) */
#include "names.h"

extern int DAT_800be09c[];
extern int DAT_800dc94c;

void FUN_8004cf7c(void);

#ifdef PC_PORT
/* the top of PS1 RAM the game uses: 2 MB, or 8 MB with the Dreamcast data (platform/pc) */
extern unsigned pc_ram_top;
#endif

void Init_ResetMemory(int a) {
	int v = DAT_800dc94c;

#ifdef PC_PORT
	DAT_800be09c[3] = pc_ram_top - 0x2000;
#else
	DAT_800be09c[3] = 0x801fe000;
#endif
	DAT_800be09c[0] = v;
	v += a;
	DAT_800be09c[1] = v;
	DAT_800be09c[2] = v;
	FUN_8004cf7c();
}
