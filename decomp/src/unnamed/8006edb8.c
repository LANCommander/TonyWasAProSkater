/* FUN_8006edb8 (0x8006edb8) */
#include "names.h"
typedef struct {
	char pad[20];
	int *p14;
	char q[64 - 24];
} S;

extern S PSXRegion[];
extern int *Memories[];

void Mem_Delete(int *);

void FUN_8006edb8(int i) {
	int *d = PSXRegion[i].p14;
	int *s = Memories[i];

	do {
		int j = 0;

		goto test;
	body:
		*d++ = *s++;
	test:
		if (j < 7500) {
			j++;
			goto body;
		}
	} while (0);

	Mem_Delete(Memories[i]);

	Memories[i] = 0;
}
