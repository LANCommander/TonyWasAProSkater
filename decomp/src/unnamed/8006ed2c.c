/* FUN_8006ed2c (0x8006ed2c) */
#include "names.h"
/* Copy of FUN_8006edb8 in the other direction; the loop keeps its entry jump, which comes from
   a goto loop (cc1 rotates while/for loops). */
typedef struct {
	char pad[20];
	int *p14;
	char q[64 - 24];
} S;

extern S PSXRegion[];
extern int *Memories[];

int *Mem_NewX(int, int, int);

#ifdef PC_PORT
/* A Dreamcast model (version 6, platform/pc/include/pc_dctex.h) is bigger than the 30000 bytes
   the front end keeps a copy of: its whole heap block (header word: size << 4) */
static int CopyWords(int *p) {
	unsigned n = ((unsigned *)p)[-1] >> 4;

	return (p[0] & 0xffff) == 6 && n > 30000 && n < 0x800000 ? (int)(n / 4) : 7500;
}
#endif

void FUN_8006ed2c(int i) {
	int *s = PSXRegion[i].p14;
#ifdef PC_PORT
	int n = CopyWords(s);
	int *d = Mem_NewX(n * 4, 1, 1);
#else
	int *d = Mem_NewX(30000, 1, 1);
#endif
	int j = 0;

	Memories[i] = d;
	goto test;
body:
	*d++ = *s++;
	j++;
test:
#ifdef PC_PORT
	if (j < n)
#else
	if (j < 7500)
#endif
		goto body;
}
