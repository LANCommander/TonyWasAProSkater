/* M3dMaths_TransformVec0__FPUs (0x8008b75c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* Multiplies a vector by the matrix m (nine halfwords, 4.12), which becomes the current
 * rotation; the result becomes the last vector (lastvec.h). Retail took the vector from the
 * GTE's V0, which its callers had loaded; in the port they pass it in the last vector, which
 * makes this the same operation as M3dMaths_TransformLastVec.
 * test_asm: lastvec v0 */
void M3dMaths_TransformVec0__FPUs(unsigned short *m) {
	geom_load_matrix(pc_geom.rot, (const int16_t *)m);
	M3dMaths_LastVec = geom_mvmva(pc_geom.rot, 0, M3dMaths_LastVec.ir[0],
	                              M3dMaths_LastVec.ir[1], M3dMaths_LastVec.ir[2], 12, 0, 0);
}
