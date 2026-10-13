/* FUN_800604c8 (0x800604c8) */
#include "names.h"

typedef struct {
	int v[5];
} T;

extern T DAT_800a1e4c;

int Front_RunMenu(T *a, T *b, int c, int d, int *e, int f, int g, int h, int i);
int FUN_800609c8(int a);
int FUN_800605c0(int a);

int FUN_800604c8(int arg) {
	int r;
	T a;
	T b;

	for (;;) {
		a = DAT_800a1e4c;
		b.v[0] = 0;
		b.v[1] = 0;
		b.v[2] = 0;
		b.v[3] = 0;
		b.v[4] = 0;
		r = Front_RunMenu(&a, &b, 5, 96, &arg, 0, 0, 0, 0);

		if (r == -1)
			goto out;

		switch (r) {
			case 97:
				r = FUN_800609c8(arg);
				break;
			case 98:
				r = FUN_800605c0(arg);
				break;
		}

		if (r != -1)
			goto out;

		arg = 0;
	}
out:
	return r;
}
