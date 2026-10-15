/* Shatter_Item__FP4Itemii (0x80021ecc) */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

int Redbook_Rnd(int);
int Shatter_Face(Item *, uint *, int, int, int, int, int);

int Shatter_Item(Item *it, int a, int b) {
	int cnt = 0, i = 0, res = 0;
	SModel *m;
	char *s;
	unsigned short n0, n1, n;
	int r;

	m = PSXRegion[it->Region].ppModels[it->Model];
	n0 = m->NumVertices;
	n1 = m->NumNormals;
	n = m->NumFaces;
	/* the faces (model file data) follow the vertices and normals */
	s = (char *)m + (n0 * 8 + 0x1c) + n1 * 8;

	for (; i < n; i++) {
		r = Shatter_Face(it, (uint *)s, Redbook_Rnd(5) == 0, 0, a, b, cnt < 6);

		if (r) {
			res = 1;

			if (r == 1)
				cnt++;
		}

		s += *(unsigned short *)(s + 2) & 0xfffc;
	}

	if (b && res) {
		it->Flags |= 1;
	}

	return res;
}
