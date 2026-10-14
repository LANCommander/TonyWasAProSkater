/* M3dMaths_LastVecMagnitude__Fv (0x8008b79c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* Length of the last vector (lastvec.h): squares the components, which also leaves the squares
 * as the last vector (clamped to 0..0x7fff, the sums unclamped), and returns the square root of
 * their sum (retail tail-jumps into M3dMaths_SquareRoot0).
 * test_asm: lastvec ir */
int M3dMaths_LastVecMagnitude__Fv(void) {
	M3dMaths_LastVec = geom_sqr(M3dMaths_LastVec.ir, 0, 1, 0);
	return M3dMaths_SquareRoot0__Fi((int)((uint32_t)M3dMaths_LastVec.mac[0] +
	                                      (uint32_t)M3dMaths_LastVec.mac[1] +
	                                      (uint32_t)M3dMaths_LastVec.mac[2]));
}
