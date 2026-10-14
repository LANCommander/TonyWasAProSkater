/* Exp_HitEnvItem__FP4ItemPUii (0x8004e968) */
#include "names.h"

typedef struct Item {
	unsigned short flags;
	char _2[0x14];
	unsigned short model;
	char _18[3];
	unsigned char slot;
	struct Item *next;
} Item;

typedef struct {
	unsigned short Flags;
} SModel;

typedef struct {
	char _0[0x10];
	SModel **models;
	char _14[0x2c];
} Slot;

extern Item *EnviroList __attribute__((section(".data")));

extern Slot PSXRegion[];

void Shatter_Item(Item *, int, int);
void Shatter_Face(Item *, unsigned *, int, int, int, int, int);

void Exp_HitEnvItem(Item *pItem, unsigned *pFace, int Damage) {
	Item *pScan;
	SModel *pModel;

	if (!pItem)
		return;

	pScan = EnviroList;

	while (pScan) {
		if (pScan == pItem)
			break;

		pScan = pScan->next;
	}

	if (!pScan)
		return;

	pModel = PSXRegion[pItem->slot].models[pItem->model];

	if (pModel->Flags & 2) {
		unsigned char Life =
		    pModel->Flags >> 16; /* always 0: a life count read from bits ushort Flags lacks */

		if (Life || Damage == 0xffff)
			if (Damage >= Life)
				Shatter_Item(pItem, 1, 1);
	} else if (pFace) {
		Shatter_Face(pItem, pFace, 1, 1, 1, 1, 1);
	}
}
