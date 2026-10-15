/* RenderModelFast__FP6SModelP6VECTOR (0x80084dc0) */
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

int FUN_80089790(E *, int, VECTOR *);
void FUN_8009736c(int, int, int);
void FUN_800898ac(E *, E *, int);

#include <stdio.h>
#include <stdlib.h>

void RenderModelFast(P *p, VECTOR *c) {
	E *v = p->v;
	int n = p->n;
	int r = FUN_80089790(v, n, c);
	E *w;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80084dc0 f%u p=%p n=%d c=%p r=%08x %s\n", pc_heartbeat, (void *)p, n, (void *)c, r,
		        (r & 0x6000e000) ? "CULLED" : "drawn");
	}

	v += n;

	if (!(r & 0x6000e000)) {
		n = p->m;
		w = v + p->i;
		FUN_8009736c(FarColor, FarColor_2, FarColor_4);
		FUN_800898ac(w, v, n);
	}
}
