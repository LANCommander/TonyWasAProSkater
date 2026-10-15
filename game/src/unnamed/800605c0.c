/* FUN_800605c0 (0x800605c0) */
#include "names.h"

typedef struct {
	unsigned char b[14];
} R;

typedef struct {
	char *w[7];
} T;

struct VerticalMenu;

extern R GCareer_c[];
extern T DAT_800a1eb4;

int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);
int FUN_80060738(int, int *);

int FUN_800605c0(int a) {
	int cnt[11];
	T t;
	int k[7];
	int i, j, v, r;

	for (i = 0; i < 11; i++) {
		cnt[i] = 0;

		for (j = 0; j < 9; j++) {
			cnt[i] += (GCareer_c[i].b[j] >> 5) & 1;
		}
	}

	for (;;) {
		t = DAT_800a1eb4;
		k[0] = 0;
		k[6] = 0;
		k[1] = cnt[0] < 3;
		k[2] = cnt[1] < 3;
		k[3] = cnt[2] < 3;
		k[4] = cnt[3] < 3;
		k[5] = cnt[4] < 3;
		v = Front_RunMenu(t.w, k, 7, 0x620, &a, 0, 0, 0, 0);

		if (v != -1) {
			if (v == 0x627)
				v = FUN_80060738(a, cnt);

			if (v == -1) {
				a = 0;
				continue;
			}
		}

		r = v;
		break;
	}

	return r;
}
