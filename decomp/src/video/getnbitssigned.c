/* GetNBitsSigned__FP10SVideoInfoi (0x80068cec) */
#include "names.h"

typedef struct SVideoInfo SVideoInfo;

unsigned GetNBitsUnsigned(SVideoInfo *, int);

int GetNBitsSigned(SVideoInfo *v, int n) {
	unsigned x = GetNBitsUnsigned(v, n);
	unsigned m = 1 << (n - 1);

	if (x & m)
		x |= -(1 << n);

	return x;
}
