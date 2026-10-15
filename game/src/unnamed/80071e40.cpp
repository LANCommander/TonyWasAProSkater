/* Init_ResetMemory__Fi (0x80071e40) */
/* The game's two heaps over the memory the port gives it (PC_GameHeap): the small one (a bytes)
   from the bottom, the big one after it up to 8 KB short of the end, as retail had them between
   .bss and its stack. */
#include "game/mem.h"
#include "pc_runtime.h"

void Init_ResetMemory(int a) {
	uint32_t size;
	char *v = (char *)PC_GameHeap(&size);

	DAT_800be09c[1].End = v + size - 0x2000;
	DAT_800be09c[0].Start = v;
	v += a;
	DAT_800be09c[0].End = v;
	DAT_800be09c[1].Start = v;
	FUN_8004cf7c();
}
