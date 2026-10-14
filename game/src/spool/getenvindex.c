/* Spool_GetEnvIndex__Fi (0x8007043c) */
#include "names.h"

int EnvRegions, EnvRegions_4;

int Spool_GetEnvIndex(int i) {
	if (i == EnvRegions)
		return 0;

	if (i == EnvRegions_4)
		return 1;

	return -1;
}
