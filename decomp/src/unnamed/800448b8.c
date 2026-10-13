/* FUN_800448b8 (0x800448b8) */
#include "names.h"

extern int HashTable[];

int *CommandPoints;

void Mem_Delete(int *p);

void FUN_800448b8(void) {
	int i;
	int *p;
	int *n;

	for (i = 0; i < 256; i++)
		HashTable[i] = 0;

	for (p = CommandPoints; p; p = n) {
		n = (int *)p[5];
		Mem_Delete(p);
	}

	CommandPoints = 0;
}
