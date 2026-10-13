/* M3dMaths_TransformVec0__FPUs (0x8008b75c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Loads the matrix m (nine halfwords) as the GTE rotation matrix and multiplies the vector
 * already in the GTE's V0 by it (rtv0, sf=1). The result becomes the "last vector" in IR1..3
 * (and MAC1..3) for the other M3dMaths_*LastVec* routines. */
void M3dMaths_TransformVec0__FPUs(unsigned short *m) {
	gte_SetRotMatrix(m);
	gte_rtv0();
}
