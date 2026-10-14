/* Shatter_Item__FP4Itemii (0x80021ecc) */
#include "names.h"

typedef struct {
	unsigned char b[64];
} Ent;

typedef struct {
	char pad[0x10];
	char **arr;
	char rest[0x2c];
} Mdl;

extern Mdl PSXRegion[];

int Redbook_Rnd(int);
int Shatter_Face(char *, char *, int, int, int, int, int);

int Shatter_Item(char *it, int a, int b) {
	int cnt = 0, i = 0, res = 0;
	char *m;
	char *s;
	unsigned short n0, n1, n;
	int r;

	m = PSXRegion[*(unsigned char *)(it + 0x1b)].arr[*(unsigned short *)(it + 0x16)];
	n0 = *(unsigned short *)(m + 2);
	n1 = *(unsigned short *)(m + 4);
	n = *(unsigned short *)(m + 6);
	s = m + (n0 * 8 + 0x1c) + n1 * 8;

	for (; i < n; i++) {
		r = Shatter_Face(it, s, Redbook_Rnd(5) == 0, 0, a, b, cnt < 6);

		if (r) {
			res = 1;

			if (r == 1)
				cnt++;
		}

		s += *(unsigned short *)(s + 2) & 0xfffc;
	}

	if (b && res) {
		*(unsigned short *)it |= 1;
	}

	return res;
}
