/* FUN_80084d18 (0x80084d18) */
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

int FUN_80085d28(E *, int);
void FUN_80086500(int, int);
void FUN_8009736c(int, int, int);
void FUN_80086f40(E *, E *, int);

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif

void FUN_80084d18(P *p) {
	E *v = p->v;
	int n = p->n;
	int r = FUN_80085d28(v, n);
	E *w;

#ifdef PC_PORT
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80084d18 f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}
#endif

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
