/* GetNBitsUnsigned__FP10SVideoInfoi (0x80068c30) */
#include "names.h"

typedef struct {
	int state;
	unsigned *f4, *f8, *fc; /* pBlockStart, pStreamStart, pStreamEnd */
	unsigned *p;
	unsigned buf;
	int n;
} SVideoInfo;

unsigned GetNBitsUnsigned(SVideoInfo *v, int n) {
	unsigned r;
	int a;

	if (v->n == 0) {
		v->buf = *v->p++;
		v->n = 32;
	}

	a = v->n;

	if (a >= n) {
		v->n = a - n;
		r = v->buf & ((1 << n) - 1);
		v->buf >>= n;
	} else {
		n -= a;
		r = v->buf & ((1 << a) - 1);
		v->buf = *v->p++;
		v->n = 32 - n;
		r |= (v->buf & ((1 << n) - 1)) << a;
		v->buf >>= n;
	}

	return r;
}
