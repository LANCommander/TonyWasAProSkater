/* M3dMaths_LastVecMagnitude__Fv (0x8008b79c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Length of the "last vector" in the GTE's IR1..3: squares the components with SQR (sf=0,
 * which also leaves the squares, clamped to 0..0x7fff, in IR1..3) and returns the square root of
 * MAC1 + MAC2 + MAC3 (retail tail-jumps into M3dMaths_SquareRoot0). */
int M3dMaths_LastVecMagnitude__Fv(void) {
	gte_sqr0();
	return M3dMaths_SquareRoot0__Fi(
	    (int)(PC_GteGetData(25) + PC_GteGetData(26) + PC_GteGetData(27)));
}
