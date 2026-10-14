/* Spool_GetModel__FUii (0x8006ee38) */
#include "names.h"

typedef struct {
	char name[10];
	unsigned char f0a, f0b;
	unsigned *models;
	unsigned *f10;
	char pad[0x2c];
} SRegion;

extern SRegion PSXRegion[];

int Spool_GetModel(unsigned id, int region) {
	unsigned n = PSXRegion[region].f10[-1];
	unsigned *m = PSXRegion[region].models;
	unsigned i;

	for (i = 0; i < n; i++) {
		if (*m == id)
			return i;

		m++;
	}
}
