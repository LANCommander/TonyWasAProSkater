/* RenderBackgroundModel__FP6SModel (0x80084e50) */
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

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif

void RenderBackgroundModel(P *p) {
	E *v = p->v;
	int n = p->n;
	int r = FUN_80086590(v, n);

	v += n;

#ifdef PC_PORT
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "bgmodel(rot) f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}
#endif

	if (!(r & 0xbf)) {
		n = p->m;
		FUN_80086f40(v + p->i, v, n);
	}
}
