/* PutNBits__FP10SVideoInfoli (0x80068b34) */
#include "names.h"

typedef struct {
	int state;
	unsigned *f4, *f8, *fc; /* pBlockStart, pStreamStart, pStreamEnd */
	unsigned *p;
	unsigned buf;
	int n;
} SVideoInfo;

void PutNBits(SVideoInfo *v, int val, int n) {
	if (n == 0)
		return;

	if (v->n < n) {
		n -= v->n;
		v->buf = (v->buf & ((1 << (32 - v->n)) - 1)) | (val << (32 - v->n));
		val >>= v->n;
		*v->p++ = v->buf;
		v->buf = 0;
		v->n = 32;
	}

	{
		int a = v->n;

		v->n = a - n;
		v->buf = (v->buf & ((1 << (32 - a)) - 1)) | (val << (32 - a));
	}

	if (v->n == 0) {
		*v->p++ = v->buf;
		v->buf = 0;
		v->n = 32;
	}
}
