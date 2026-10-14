/* FUN_8004cf7c (0x8004cf7c) */
#include "names.h"

typedef struct B {
	struct B *next;
	unsigned flags : 4;
	unsigned len : 28;
} B;

typedef struct {
	B *a;
	unsigned b;
} R;

extern R DAT_800be09c[];

int Used[2];
int FirstFreeBlock[2];
unsigned CriticalBigHeapUsage;

void AddToFreeList(B *b, int i);

void FUN_8004cf7c(void) {
	int i;

	for (i = 0; i < 2; i++) {
		FirstFreeBlock[i] = 0;
		Used[i] = 0;

		{
			B *b = DAT_800be09c[i].a;

			b->len = DAT_800be09c[i].b - (unsigned)b - 8;
			AddToFreeList(b, i);
		}
	}

	CriticalBigHeapUsage = (98 * (DAT_800be09c[1].b - (unsigned)DAT_800be09c[1].a)) / 100;
}
