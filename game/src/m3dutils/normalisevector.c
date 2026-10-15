/* M3dUtils_NormaliseVector (0x80081f24): shrink into GTE range, SQR+SquareRoot0, scale to 4096. */
#include <sys/types.h>
#include <libgte.h>

typedef struct {
	int x, y, z;
} V;

extern V *__ars__7CVectorRCi(V *, const int *);
extern int SquareRoot0(int);

void M3dUtils_NormaliseVector(V *v) {
	VECTOR t;
	int len;

	while (v->x > 26754 || v->x < -26754 || v->y > 26754 || v->y < -26754 || v->z > 26754 ||
	       v->z < -26754) {
		int one = 1;

		__ars__7CVectorRCi(v, &one);
	}

	geom_square(v, &t);

	len = SquareRoot0(t.vx + t.vy + t.vz);

	if (len) {
		int x = (v->x << 12) / len, y = (v->y << 12) / len, z = (v->z << 12) / len;

		v->x = x;
		v->y = y;
		v->z = z;
	} else {
		v->x = 4096;
		v->y = 0;
		v->z = 0;
	}
}
