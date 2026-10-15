/* Spool_BackupPSX__Fi (0x8006ed2c) */
#include "game/m3d.h"
#include "game/mem.h"

/* Copy of Spool_RestorePSX in the other direction; the loop keeps its entry jump, which comes from
   a goto loop (cc1 rotates while/for loops). */
extern SPSXRegion PSXRegion[];
extern uint *Memories[];

/* A Dreamcast model (version 6, platform/pc/include/pc_dctex.h) is bigger than the 30000 bytes
   the front end keeps a copy of: its whole heap block */
static int CopyWords(uint *p) {
	uint n = MEM_INFO((char *)p - MEM_HEADER) >> 4;

	return (p[0] & 0xffff) == 6 && n > 30000 && n < 0x800000 ? (int)(n / 4) : 7500;
}

void Spool_BackupPSX(int i) {
	uint *s = PSXRegion[i].pPSX;
	int n = CopyWords(s);
	uint *d = (uint *)Mem_NewX(n * 4, 1, 1);
	int j = 0;

	Memories[i] = d;
	goto test;
body:
	*d++ = *s++;
	j++;
test:
	if (j < n)
		goto body;
}
