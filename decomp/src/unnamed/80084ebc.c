/* RenderModelInSuper__FP6SModel (0x80084ebc) */
#include "names.h"

typedef struct {
	int a, b;
} E;

typedef struct {
	short a;
	unsigned short n;
	unsigned short i;
	unsigned short m;
	char pad[20];
	E v[1];
} P;

int Dewibble;
int TestTheWater;
int SubObjectMasked;
int WaterClipValue;

int FUN_800860cc(E *, int);
int FUN_80085e98(E *, int);
void FUN_80086500(int, int);
void FUN_80086f40(E *, E *, int);

void RenderModelInSuper(P *p) {
	int n = p->n;
	E *v = p->v;
	int r;

	if (Dewibble)
		r = FUN_800860cc(v, n);
	else
		r = FUN_80085e98(v, n);

	if (TestTheWater)
		FUN_80086500(n, WaterClipValue);

	v += n;

	if (!SubObjectMasked && !(r & 0xbf))
		FUN_80086f40(v + p->i, v, p->m);
}
