/* Bang__FUcUcUc (0x8004ce28) */
/* The game's fatal-error handler: on the PS1 it fills the screen with a colour and spins. The port
   reports which heap ran out (each heap's free list) and stops (PC_Bang). */
#include "game/mem.h"
#include "pc_runtime.h"

void Bang(uchar r, uchar g, uchar b) {
	int heap;

	PC_Log("Bang(%d, %d, %d): the game hit a fatal error (out of memory if from Mem_New*)", r, g, b);

	for (heap = 0; heap < 2; heap++) {
		SBlockHeader *pBlock;
		uint total = 0, n = 0, big = 0;

		for (pBlock = FirstFreeBlock[heap]; pBlock && n < 100000; pBlock = pBlock->pNext, n++) {
			total += pBlock->Size;

			if (pBlock->Size > big)
				big = pBlock->Size;
		}

		PC_Log("  heap %d: %u free blocks, %u bytes free, largest %u; used %d", heap, n, total,
		       big, Used[heap]);
	}

	PC_Bang(r, g, b);
}
