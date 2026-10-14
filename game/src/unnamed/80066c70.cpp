/* LevelSelect_FreePackSlots__Fv (0x80066c70) */
/* C++. Frees everything in both pack slots: selects the default slot, then for each slot
   unpacks its pack and walks its two allocation lists, freeing every node. */
#include "game/levelselect.h"
#include "game/mem.h"

struct SNode {
	int a, b;
	SNode *pNext;
};

SNode *DAT_800d2398[2];
SNode *DAT_800d23a0[2];
int DAT_800d23b0[2];
int DAT_800d23b8[2];

void Pack_Unpack(short, short);

void LevelSelect_FreePackSlots() {
	int i;

	LevelSelect_SelectPackSlot(-1);

	for (i = 0; i < 2; i++) {
		Pack_Unpack(DAT_800d23b0[i], DAT_800d23b8[i]);

		while (DAT_800d2398[i]) {
			SNode *n = DAT_800d2398[i]->pNext;

			Mem_Delete(DAT_800d2398[i]);

			DAT_800d2398[i] = n;
		}

		while (DAT_800d23a0[i]) {
			SNode *n = DAT_800d23a0[i]->pNext;

			Mem_Delete(DAT_800d23a0[i]);

			DAT_800d23a0[i] = n;
		}
	}
}
