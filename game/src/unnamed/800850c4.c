/* RenderBackgroundModelNonRotated__FP6SModel (0x800850c4) */
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

int FUN_80086590(E *, int);
void FUN_80086f40(E *, E *, int);

#include <stdio.h>
#include <stdlib.h>

void RenderBackgroundModelNonRotated(P *p) {
	E *v = p->v;
	int n = p->n;
	int r = FUN_80086590(v, n);

	v += n;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "bgmodel f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (!(r & 0xbf)) {
		n = p->m;
		FUN_80086f40(v + p->i, v, n);
	}
}
