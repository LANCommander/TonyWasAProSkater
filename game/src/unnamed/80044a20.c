/* Trig_TriggerCommandPoint__FUib (0x80044a20) */
#include "names.h"

/* Fires the trigger with the given id: marks it hit for this side (1 or 2) and, the first time
   for that side, runs its script. Nodes are hashed on the id's low byte (see FUN_80044820). */
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
extern void *pCurrentPlayer;
extern void *GSkater;

void ExecuteCommandList(char *, int, int);

STrig *Trig_TriggerCommandPoint(int id) {
	STrig *t;

	for (t = HashTable[id & 0xff]; t; t = t->pNext) {
		if (t->Id == id) {
			int bit = 2;

			if (pCurrentPlayer == GSkater)
				bit = 1;

			t->Hit |= bit;

			if (!(t->Done & bit)) {
				ExecuteCommandList(t->pScript, t->Line, 0);

				t->Done |= bit;
				return t;
			}
		}
	}

	return 0;
}
