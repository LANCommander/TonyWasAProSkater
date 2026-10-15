/* Exp_HitEnvItem__FP4ItemPUii (0x8004e968) */
#include "game/m3d.h"

extern Item *EnviroList;
extern SPSXRegion PSXRegion[40];

void Shatter_Item(Item *, int, int);
void Shatter_Face(Item *, uint *, int, int, int, int, int);

void Exp_HitEnvItem(Item *pItem, uint *pFace, int Damage) {
	Item *pScan;
	SModel *pModel;

	if (!pItem)
		return;

	pScan = EnviroList;

	while (pScan) {
		if (pScan == pItem)
			break;

		pScan = pScan->NextItem;
	}

	if (!pScan)
		return;

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

	if (pModel->Flags & 2) {
		uchar Life =
		    pModel->Flags >> 16; /* always 0: a life count read from bits ushort Flags lacks */

		if (Life || Damage == 0xffff)
			if (Damage >= Life)
				Shatter_Item(pItem, 1, 1);
	} else if (pFace) {
		Shatter_Face(pItem, pFace, 1, 1, 1, 1, 1);
	}
}
