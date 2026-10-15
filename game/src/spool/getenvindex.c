/* Spool_GetEnvIndex__Fi (0x8007043c) */
#include "names.h"

extern int EnvRegions[2];

int Spool_GetEnvIndex(int i) {
	if (i == EnvRegions[0])
		return 0;

	if (i == EnvRegions[1])
		return 1;

	return -1;
}
