/* GetBit__FP10SVideoInfo (0x80068d38) */
#include "names.h"

typedef struct {
	int state;
	unsigned *f4, *f8, *fc; /* pBlockStart, pStreamStart, pStreamEnd */
	unsigned *p;
	unsigned buf;
	int n;
} SVideoInfo;

int GetBit(SVideoInfo *v) {
	int b;

	if (v->n == 0) {
		v->buf = *v->p++;
		v->n = 32;
	}

	v->n--;
	b = v->buf & 1;
	v->buf >>= 1;
	return b;
}
