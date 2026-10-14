/* Init_ResetMemory__Fi (0x80071e40) */
#include "names.h"
#include "pc_runtime.h"

extern int DAT_800be09c[];

void FUN_8004cf7c(void);

/* The game's two heaps over the memory the port gives it (PC_GameHeap): the small one (a bytes)
   from the bottom, the big one after it up to 8 KB short of the end, as retail had them between
   .bss and its stack. */
void Init_ResetMemory(int a) {
	uint32_t size;
	unsigned char *v = PC_GameHeap(&size);

	DAT_800be09c[3] = (int)(uintptr_t)(v + size - 0x2000);
	DAT_800be09c[0] = (int)(uintptr_t)v;
	v += a;
	DAT_800be09c[1] = (int)(uintptr_t)v;
	DAT_800be09c[2] = (int)(uintptr_t)v;
	FUN_8004cf7c();
}
