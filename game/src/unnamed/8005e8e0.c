/* FUN_8005e8e0 (0x8005e8e0) */
#include "names.h"

typedef struct {
	int a, b, c;
} V;

extern V DAT_800a1c84;

typedef struct {
	char p[0xd4];
	int f;
} Q;

extern Q GameText;
extern int GHasCareerLoaded;

int DAT_800d1d80;

int Front_RunMenu(V *, int *, int, int, int *, int, int, int, int);
void FUN_8005e4ec(int *, int, int);
int Front_MemCardScreen(int);

int FUN_8005e8e0(int a) {
	V l;
	int k[3];
	int s;
	int v, r;

	for (;;) {
		l = DAT_800a1c84;
		k[1] = 0;
		k[2] = 0;
		k[0] = DAT_800d1d80 ^ 1;
		v = Front_RunMenu(&l, k, 3, 16, &a, 0, 0, 0, 0);

		if (v == -1)
			goto ret;

		switch (v) {
			case 18:
				break;
			case 19:
				v = Front_MemCardScreen(0);

				if (v == 0) {
					v = GameText.f;
					s = v;
					FUN_8005e4ec(&s, 1, -1);

					v = -1;
				}

				break;
			case 17:
				GHasCareerLoaded = 1;
				break;
		}

		if (v == -1) {
			a = 0;
			continue;
		}
	ret:
		r = v;
		break;
	}

	return r;
}
