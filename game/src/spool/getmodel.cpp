/* Spool_GetModel__FUii (0x8006ee38) */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

/* (retail read the count from before the file's model table, which ppModels pointed at; the
   port's ppModels is an array of its own, the count is NumModels) */
int Spool_GetModel(uint id, int region) {
	uint n = PSXRegion[region].NumModels;
	uint *m = PSXRegion[region].pModelChecksums;
	uint i;

	for (i = 0; i < n; i++) {
		if (*m == id)
			return i;

		m++;
	}
}
