/* FUN_800448b8 (0x800448b8) */
#include "names.h"

/* SCommandPoint (game/trig.h) */
typedef struct CP {
	unsigned short *pCommands;
	unsigned char b4[4];
	unsigned short h8[2];
	unsigned int Checksum;
	struct CP *pNextSimilar;
	struct CP *pNext;
} CP;

extern CP *HashTable[];

CP *CommandPoints;

void Mem_Delete(void *p);

void FUN_800448b8(void) {
	int i;
	CP *p;
	CP *n;

	for (i = 0; i < 256; i++)
		HashTable[i] = 0;

	for (p = CommandPoints; p; p = n) {
		n = p->pNext;
		Mem_Delete(p);
	}

	CommandPoints = 0;
}
