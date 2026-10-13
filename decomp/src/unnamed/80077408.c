/* QFromV__FRC7CVector (0x80077408) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

extern char DAT_800a3474[];

typedef struct {
	short s, c;
} ST;

extern ST rcossin_tbl[];

int M3dMaths_SquareRoot0__Fi(int a);
void VectorNormal(V *a, V *b);

char *QFromV(char *out, V *v) {
	int s;
	V t;
	int a, b, x, y, z;

	s = M3dMaths_SquareRoot0__Fi(v->x * v->x + v->y * v->y + v->z * v->z);

	if (!s) {
		*(char **)out = DAT_800a3474;
		*(int *)(out + 4) = 0;
		*(int *)(out + 8) = 0;
		*(int *)(out + 12) = 0;
		*(int *)(out + 16) = 4096;
	} else {
		VectorNormal(v, &t);

		{
			ST *bb;
			int i = (s >> 1) & 4095;

			bb = rcossin_tbl;
			a = bb[i].s;
			b = bb[i].c;
		}

		x = (a * t.x) >> 12;
		y = (a * t.y) >> 12;
		z = (a * t.z) >> 12;
		*(char **)out = DAT_800a3474;
		*(int *)(out + 16) = b;
		*(int *)(out + 4) = x;
		*(int *)(out + 8) = y;
		*(int *)(out + 12) = z;
	}

	return out;
}
