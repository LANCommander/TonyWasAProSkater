/* Spool_ClearPSX__FPc (0x8006fd18) */
#include "names.h"

typedef struct {
	char name[10];
	unsigned char f0a, f0b;
	unsigned *models;
	unsigned *f10;
	char pad[0x2c];
} SRegion;

extern SRegion PSXRegion[];

int Spool_FindRegion(char *);
void ClearRegion(int, int, int);

void Spool_ClearPSX(char *name) {
	int r = Spool_FindRegion(name);

	if (PSXRegion[r].f0a)
		ClearRegion(Spool_FindRegion(name), 1, 0);
}
