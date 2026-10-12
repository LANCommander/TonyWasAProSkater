/* Pack_MergeAdjacent__FP6TSPackT0 (0x8004a388) */
#include "game/m3d.h"

TSPack *Pack_pFree;

void Pack_Remove(TSPack **, TSPack *);
TSPack *Pack_NewFree(int, int, int, int);

TSPack *Pack_MergeAdjacent(TSPack *p_first, TSPack *p_second) {
	int x, y, w, h;

	if (p_first->Y == p_second->Y) {
		if (p_first->X < p_second->X) {
			if (p_first->X + p_first->W == p_second->X && p_first->H == p_second->H) {
				x = p_first->X;
				y = p_first->Y;
				w = p_first->W + p_second->W;
				h = p_first->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		} else if (p_second->X < p_first->X) {
			if (p_second->X + p_second->W == p_first->X && p_first->H == p_second->H) {
				x = p_second->X;
				y = p_second->Y;
				w = p_first->W + p_second->W;
				h = p_first->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		}
	} else if (p_first->X == p_second->X) {
		if (p_first->Y < p_second->Y) {
			if (p_first->Y + p_first->H == p_second->Y && p_first->W == p_second->W) {
				x = p_first->X;
				y = p_first->Y;
				w = p_first->W;
				h = p_first->H + p_second->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		} else if (p_second->Y < p_first->Y) {
			if (p_second->Y + p_second->H == p_first->Y && p_first->W == p_second->W) {
				x = p_second->X;
				y = p_second->Y;
				w = p_first->W;
				h = p_first->H + p_second->H;
				Pack_Remove(&Pack_pFree, p_first);
				Pack_Remove(&Pack_pFree, p_second);
				return Pack_NewFree(x, y, w, h);
			}
		}
	}
}
