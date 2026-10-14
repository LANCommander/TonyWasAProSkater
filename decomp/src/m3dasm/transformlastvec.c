/* M3dMaths_TransformLastVec__FPUs (0x8008b71c) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* The M3DMATHS "last vector" routines keep their working vector between calls (lastvec.h). */
GeomAcc M3dMaths_LastVec;

/* Multiplies the last vector by the matrix m (nine halfwords, 4.12), which becomes the current
 * rotation: last vector = m * last vector >> 12.
 * test_asm: lastvec ir */
void M3dMaths_TransformLastVec__FPUs(unsigned short *m) {
	geom_load_matrix(pc_geom.rot, (const int16_t *)m);
	M3dMaths_LastVec = geom_mvmva(pc_geom.rot, 0, M3dMaths_LastVec.ir[0],
	                              M3dMaths_LastVec.ir[1], M3dMaths_LastVec.ir[2], 12, 0, 0);
}
