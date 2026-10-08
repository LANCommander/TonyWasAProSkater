/* __11CSmokeTrailR7CVectoriiii (0x8002a2ec) */
#include "names.h"
typedef struct {
	char p0[0x44];
	char *vt;
	int n;
	char p1[0x5c - 0x4c];
	int *arr;
} B;

extern char __vt_11CSmokeTrail[];
extern char s_Ribbon[];

void CRibbon_ctor(B *, int, int, int, char *, char *, int, int);
void CFT4Bit_SetTint(int, int, int, int);
void CFT4Bit_SetTransDecay(int, int);

B *CSmokeTrail_ctor(B *p, int a1, int a2, int r, int g, int b) {
	int i, j, t, n, dr, dg, db;

	CRibbon_ctor(p, a1, a2, 2, s_Ribbon, s_Ribbon, 400, 1);

	n = p->n;
	dr = r / n;
	dg = g / n;
	i = 0;
	db = b / n;
	p->vt = __vt_11CSmokeTrail;

	if (n > 0) {
		do {
			j = i + 1;
			t = n - j;
			CFT4Bit_SetTint(p->arr[i], (r - t * dr) & 255, (g - t * dg) & 255,
			                        (b - t * db) & 255);
			CFT4Bit_SetTransDecay(p->arr[i], 8);

			n = p->n;
			i = j;
		} while (i < n);
	}

	return p;
}
