/* Spool_FindEnviroItem__FUi (0x8006ee88) */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

Item *EnviroList;

Item *Spool_FindEnviroItem(uint id) {
	Item *e = EnviroList;
	int cur = -1;
	uint *tbl;

	while (e) {
		if (e->Region != cur) {
			cur = e->Region;
			tbl = PSXRegion[cur].pModelChecksums;
		}

		if (tbl[e->Model] == id)
			return e;

		e = e->NextItem;
	}

	return 0;
}
