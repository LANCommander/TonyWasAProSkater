/* Spool_ClearPSX__FPc (0x8006fd18) */
#include "game/m3d.h"
#include "game/spool.h"

extern SPSXRegion PSXRegion[];

int Spool_FindRegion(char *);

void Spool_ClearPSX(char *name) {
	int r = Spool_FindRegion(name);

	if (PSXRegion[r].Usable)
		ClearRegion(Spool_FindRegion(name), 1, 0);
}
