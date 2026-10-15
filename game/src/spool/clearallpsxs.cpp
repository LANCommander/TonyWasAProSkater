/* Spool_ClearAllPSXs__Fv (0x8006fd74) */
#include "game/m3d.h"
#include "game/spool.h"

extern SPSXRegion PSXRegion[];

void Spool_ClearEnvironmentRegions();
void Spool_RemoveUnusedTextures();

void Spool_ClearAllPSXs() {
	int i;

	Spool_Sync();
	Spool_ClearEnvironmentRegions();

	for (i = 0; i < 40; i++)
		if (!PSXRegion[i].Protected)
			ClearRegion(i, 1, 0);

	Spool_RemoveUnusedTextures();
}
