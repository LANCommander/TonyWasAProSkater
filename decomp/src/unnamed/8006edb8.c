/* Spool_RestorePSX__Fi (0x8006edb8) */
#include "names.h"

typedef struct {
	char pad[20];
	int *p14;
	char q[64 - 24];
} S;

extern S PSXRegion[];
extern int *Memories[];

void Mem_Delete(int *);

#ifdef PC_PORT
/* A Dreamcast model (version 6, platform/pc/include/pc_dctex.h) is bigger than the 30000 bytes
   the front end keeps a copy of: its whole heap block (header word: size << 4) */
static int CopyWords(int *p) {
	unsigned n = ((unsigned *)p)[-1] >> 4;

	return (p[0] & 0xffff) == 6 && n > 30000 && n < 0x800000 ? (int)(n / 4) : 7500;
}
#endif

void Spool_RestorePSX(int i) {
	int *d = PSXRegion[i].p14;
	int *s = Memories[i];
#ifdef PC_PORT
	int n = CopyWords(s);

	if (CopyWords(d) < n)
		n = CopyWords(d);
#endif

	do {
		int j = 0;

		goto test;
	body:
		*d++ = *s++;
	test:
#ifdef PC_PORT
		if (j < n) {
#else
		if (j < 7500) {
#endif
			j++;
			goto body;
		}
	} while (0);

	Mem_Delete(Memories[i]);

	Memories[i] = 0;
}
