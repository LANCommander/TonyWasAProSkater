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

void FUN_8006ed2c(int i) {
	int *s = PSXRegion[i].p14;
	int *d = Mem_NewX(30000, 1, 1);
	int j = 0;

	Memories[i] = d;
	goto test;
body:
	*d++ = *s++;
	j++;
test:
	if (j < 7500)
		goto body;
}
