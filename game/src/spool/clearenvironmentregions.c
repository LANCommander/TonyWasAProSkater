/* Spool_ClearEnvironmentRegions__Fv (0x8006fefc) */
#include "names.h"

int EnvRegions, EnvRegions_4, OtherEnvIndex;

void ClearRegion(int, int, int);
void Spool_RemoveUnusedTextures(void);

void Spool_ClearEnvironmentRegions(void) {
	ClearRegion(EnvRegions, 0, 0);
	ClearRegion(EnvRegions_4, 0, 0);

	EnvRegions = -1;
	EnvRegions_4 = -1;
	Spool_RemoveUnusedTextures();

	OtherEnvIndex = 0;
}
