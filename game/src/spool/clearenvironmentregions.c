/* Spool_ClearEnvironmentRegions__Fv (0x8006fefc) */
#include "names.h"

extern int EnvRegions[2], OtherEnvIndex;

void ClearRegion(int, int, int);
void Spool_RemoveUnusedTextures(void);

void Spool_ClearEnvironmentRegions(void) {
	ClearRegion(EnvRegions[0], 0, 0);
	ClearRegion(EnvRegions[1], 0, 0);

	EnvRegions[0] = -1;
	EnvRegions[1] = -1;
	Spool_RemoveUnusedTextures();

	OtherEnvIndex = 0;
}
