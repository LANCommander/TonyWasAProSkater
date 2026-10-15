/* Spool_RestorePSX__Fi (0x8006edb8) */
#include "game/m3d.h"
#include "game/mem.h"

extern SPSXRegion PSXRegion[];
extern uint *Memories[];

/* A Dreamcast model (version 6, platform/pc/include/pc_dctex.h) is bigger than the 30000 bytes
   the front end keeps a copy of: its whole heap block */
static int CopyWords(uint *p) {
	uint n = MEM_INFO((char *)p - MEM_HEADER) >> 4;

	return (p[0] & 0xffff) == 6 && n > 30000 && n < 0x800000 ? (int)(n / 4) : 7500;
}

void Spool_RestorePSX(int i) {
	uint *d = PSXRegion[i].pPSX;
	uint *s = Memories[i];
	int n = CopyWords(s);

	if (CopyWords(d) < n)
		n = CopyWords(d);

	do {
		int j = 0;

		goto test;
	body:
		*d++ = *s++;
	test:
		if (j < n) {
			j++;
			goto body;
		}
	} while (0);

	Mem_Delete(Memories[i]);

	Memories[i] = 0;
}
