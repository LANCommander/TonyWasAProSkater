/* M3dMaths_NormaliseLastVec__Fv (0x8008b7ec) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* MIPS div: never traps. Division by zero gives lo = -1 for n >= 0 and 1 for n < 0. */
static int mips_div(int n, int d)
{
	if (d == 0)
		return n < 0 ? 1 : -1;
	return n / d;
}

/* Scales the "last vector" in the GTE's IR1..3 to unit length in 4.12 fixed point:
 * each component becomes (c << 12) / |v|, with |v| from M3dMaths_LastVecMagnitude (whose SQR
 * also leaves MAC1..3 = the squares). The results are written back to IR1..3. */
void M3dMaths_NormaliseLastVec__Fv(void)
{
	int v[3], mag, i;

	for (i = 0; i < 3; i++)
		v[i] = (int)PC_GteGetData(9 + i);
	mag = M3dMaths_LastVecMagnitude__Fv();
	for (i = 0; i < 3; i++)
		PC_GteSetData(9 + i, (uint32_t)mips_div((int)((unsigned)v[i] << 12), mag));
}
