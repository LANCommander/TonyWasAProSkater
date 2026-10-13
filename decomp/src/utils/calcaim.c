/* Utils_CalcAim__FR8CSVectorRC7CVectorT1 (0x80047ddc) */
#include "names.h"

typedef struct {
	short x, y, z;
} SV;

typedef struct {
	int x, y, z;
} V;

int FUN_80093604(int);
int FUN_8008b5dc(int);

int Utils_CalcAim(SV *o, const V *a, const V *b) {
	int dx = (b->x - a->x) >> 12;
	int dy = (b->y - a->y) >> 12;
	int dz = (b->z - a->z) >> 12;
	int d;

	if (dz != 0) {
		if (dz > 0)
			o->y = 2048 - FUN_80093604((dx << 12) / -dz);
		else
			o->y = FUN_80093604((dx << 12) / dz);
	} else {
		o->y = dx <= 0 ? 1024 : -1024;
	}

	d = FUN_8008b5dc(dx * dx + dz * dz);

	if (d != 0) {
		if (dy > 0)
			o->x = FUN_80093604((unsigned)(dy << 12) / (unsigned)d);
		else
			o->x = -FUN_80093604((unsigned)(-dy << 12) / (unsigned)d);
	} else {
		o->x = dy <= 0 ? -1024 : 1024;
	}

	o->z = 0;
	o->x &= 4095;
	o->y &= 4095;
	return d;
}
