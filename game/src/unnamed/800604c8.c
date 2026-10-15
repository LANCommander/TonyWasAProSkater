/* FUN_800604c8 (0x800604c8) */
#include "names.h"

typedef struct {
	char *v[5];
} T;

typedef struct {
	int v[5];
} K;

struct VerticalMenu;

extern T DAT_800a1e4c;

int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);
int FUN_800609c8(int a);
int FUN_800605c0(int a);

int FUN_800604c8(int arg) {
	int r;
	T a;
	K b;

	for (;;) {
		a = DAT_800a1e4c;
		b.v[0] = 0;
		b.v[1] = 0;
		b.v[2] = 0;
		b.v[3] = 0;
		b.v[4] = 0;
		r = Front_RunMenu(a.v, b.v, 5, 96, &arg, 0, 0, 0, 0);

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
