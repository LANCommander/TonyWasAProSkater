/* Spool_FindRegion__FPc (0x8006fcb0) */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

int Utils_CompareStrings(const char *, const char *, uint);

int Spool_FindRegion(char *name) {
	int i;

	for (i = 0; i < 40; i++)
		if (Utils_CompareStrings(name, PSXRegion[i].Filename, -1))
			return i;

	return -1;
}
