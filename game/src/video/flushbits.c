/* FlushBits__FP10SVideoInfo (0x80068bf8) */
#include "names.h"

typedef struct {
	int state, f4, f8, fc;
	unsigned *p;
	unsigned buf;
	int n;
} SVideoInfo;

void FlushBits(SVideoInfo *v) {
	if (v->n != 32) {
		*v->p++ = v->buf;
		v->buf = 0;
		v->n = 32;
	}
}
