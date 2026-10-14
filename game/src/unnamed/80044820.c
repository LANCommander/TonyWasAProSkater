/* CreateCommandPoint__FUiUsPUs (0x80044820) */
#include "names.h"

/* Allocates a trigger node (see Trig_TriggerCommandPoint): linked into the id's hash chain (on the id's low
   byte) and onto the list of all triggers. */
typedef struct STrig {
	char *pScript;
	unsigned char Hit;
	unsigned char Done;
	unsigned char f6, f7;
	short f8;
	unsigned short Line;
	int Id;
	struct STrig *pNext;
	struct STrig *pNextAll;
} STrig;

extern STrig *HashTable[];

STrig *CommandPoints;

void *Mem_NewX(int, int, int);

void CreateCommandPoint(int id, unsigned short line, char *script) {
	STrig *t = Mem_NewX(sizeof(STrig), 0, 1);

	t->pNextAll = CommandPoints;
	t->pNext = HashTable[id & 0xff];
	HashTable[id & 0xff] = t;
	t->Line = line;
	t->pScript = script;
	t->Id = id;
	CommandPoints = t;
	t->Hit = 0;
	t->Done = 0;
	t->f7 = 0;
	t->f6 = 0;
	t->f8 = 0;
}
