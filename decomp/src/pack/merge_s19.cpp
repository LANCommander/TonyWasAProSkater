/* Pack_Merge__FP6TSPack (0x8004a590) */
#include "game/m3d.h"

TSPack *Pack_pFree;

TSPack *Pack_MergeAdjacent(TSPack *, TSPack *);

int Pack_Merge(TSPack *p_merge) {
	TSPack *p_free;
	int worth_continuing;
	int merged;

	merged = 0;

	do {
		worth_continuing = 0;
		p_free = Pack_pFree;

		while (p_free) {
			if (p_merge->Y == p_free->Y && p_merge->H == p_free->H) {
				if (p_free->X + p_free->W == p_merge->X || p_merge->X + p_merge->W == p_free->X) {
					worth_continuing = 1;
					p_merge = Pack_MergeAdjacent(p_free, p_merge);
					merged = 1;
					break;
				}
			} else if (p_merge->X == p_free->X && p_merge->W == p_free->W &&
			           (p_free->Y & 256) == (p_merge->Y & 256)) {
				if (p_free->Y + p_free->H == p_merge->Y || p_merge->Y + p_merge->H == p_free->Y) {
					worth_continuing = 1;
					p_merge = Pack_MergeAdjacent(p_free, p_merge);
					merged = 1;
					break;
				}
			}

			p_free = p_free->pNext;
		}
	} while (worth_continuing);

	return merged;
}
