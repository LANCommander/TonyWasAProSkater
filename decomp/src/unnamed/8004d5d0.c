/* Mem_Delete__FPv (0x8004d5d0) */
#include "names.h"

typedef struct B {
	struct B *next;
	int flags : 4;
	unsigned len : 28;
} B;

B *DAT_800d1b48;
B *DAT_800d1b4c;
int LowMemory;
unsigned CriticalBigHeapUsage;
int Used[2];
unsigned Used_4;

void AddToFreeList(B *b, int i);

void Mem_Delete(char *p) {
	B *b = (B *)(p - 8);
	int f = b->flags;

	if (f == -2) {
		if (b == DAT_800d1b48) {
			DAT_800d1b48 = 0;
			return;
		}

		if (b == DAT_800d1b4c) {
			DAT_800d1b4c = 0;
			return;
		}
	}

	if ((unsigned)f < 2) {
		Used[f] -= 8 + b->len;
		AddToFreeList(b, f);

		if (f == 1) {
			if (Used_4 >= CriticalBigHeapUsage)
				LowMemory = f;
			else
				LowMemory = 0;
		}
	}
}
