/* RenderModelNonRotated__FP6SModelP6VECTOR (0x80085018) */
#include "names.h"
#include <sys/types.h>
#include <libgte.h>

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

short FarColor;
short FarColor_2;
short FarColor_4;
int TestTheWater;
int WaterClipValue;

int FUN_80086d2c(E *, int, VECTOR *);
void FUN_80086500(int, int);
void FUN_8009736c(int, int, int);
void FUN_80086f40(E *, E *, int);

#include <stdio.h>
#include <stdlib.h>

void RenderModelNonRotated(P *p, VECTOR *c) {
	E *v = p->v;
	int n = p->n;
	int r = FUN_80086d2c(v, n, c);
	E *w;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80085018 f%u p=%p n=%d c=%p r=%08x %s\n", pc_heartbeat, (void *)p, n, (void *)c, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (TestTheWater)
		FUN_80086500(n, WaterClipValue);

	v += n;

	if (!(r & 0xbf)) {
		n = p->m;
		w = v + p->i;
		FUN_8009736c(FarColor, FarColor_2, FarColor_4);
		FUN_80086f40(w, v, n);
	}
}
