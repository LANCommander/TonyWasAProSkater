/* M3dUtils_InBetween__FP6CSuper (0x80081448) */
#include "names.h"

typedef struct {
	short _0[4];
} Frame;

typedef struct {
	char _0[0x18];
	Frame *frames;
	char _1c[0x30 - 0x1c];
	unsigned short n;
	char _32[0x40 - 0x32];
} Hook;

typedef struct {
	char _0[0x1b];
	unsigned char idx;
	char _1c[0x146 - 0x1c];
	unsigned short frame;
} CSuper;

extern Hook PSXRegion[];

typedef struct {
	char _0[0xa];
	unsigned short count;
} FrameX;

extern void M3dUtils_InterpolateVectors(int, int, unsigned *, CSuper *, int, int);

void M3dUtils_InBetween(CSuper *s) {
	int i = s->idx;
	Frame *f = PSXRegion[i].frames;
	int c = ((FrameX *)&f[s->frame])->count;
	unsigned _t5683 = c + 1;

	if (c) {
		int n = PSXRegion[i].n;

		M3dUtils_InterpolateVectors(4 * n, _t5683, (unsigned *)f, s, 0, n);
	}
}
