/* Spool_ClearAllPSXs__Fv (0x8006fd74) */
#include "names.h"

typedef struct {
	char name[10];
	unsigned char f0a, f0b;
	unsigned *models;
	unsigned *f10;
	char pad[0x2c];
} SRegion;

extern SRegion PSXRegion[];

void Spool_Sync(void);
void Spool_ClearEnvironmentRegions(void);
void Spool_RemoveUnusedTextures(void);
void ClearRegion(int, int, int);

void Spool_ClearAllPSXs(void) {
	int i;

	Spool_Sync();
	Spool_ClearEnvironmentRegions();

	for (i = 0; i < 40; i++)
		if (!PSXRegion[i].f0b)
			ClearRegion(i, 1, 0);

	Spool_RemoveUnusedTextures();
}
