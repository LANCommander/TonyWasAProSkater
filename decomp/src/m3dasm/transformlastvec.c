/* M3dMaths_TransformLastVec__FPUs (0x8008b71c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* The M3DMATHS "last vector" routines keep their working vector in the GTE's IR1..IR3 between
 * calls. This one loads the matrix m (nine halfwords) as the GTE rotation matrix and multiplies
 * the last vector by it (rtir, sf=1: IR = m * IR >> 12); the result stays in IR1..3/MAC1..3. */
void M3dMaths_TransformLastVec__FPUs(unsigned short *m) {
	gte_SetRotMatrix(m);
	gte_rtir();
}
