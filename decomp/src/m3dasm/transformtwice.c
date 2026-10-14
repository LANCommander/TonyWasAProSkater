/* M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX (0x80089eac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* Transforms v by the animation frame sm (rotation + short translation), then the result by m
 * (rotation + translation): v' = Rsm*v + Tsm, then Rm*v' + Tm, each >> 12 and saturated to 16
 * bits. The point is left as M3DMATHS' last vector (lastvec.h; retail left it in the GTE) for the
 * caller to read. State left as retail leaves it: rotation + translation = sm, light matrix = m,
 * background colour = m's translation (retail's second product runs on those).
 * test_asm: lastvec out */
void M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX(struct SVector *v, struct SMatrix *sm,
                                                        MATRIX *m) {
	GeomAcc a;

	geom_load_matrix(pc_geom.rot, &sm->m[0][0]);
	geom_load_vector(pc_geom.tr, sm->t[0], sm->t[1], sm->t[2]);
	geom_load_vector(pc_geom.bk, m->t[0], m->t[1], m->t[2]);
	a = geom_mvmva(pc_geom.rot, pc_geom.tr, v->vx, v->vy, v->vz, 12, 0, 0);
	geom_load_matrix(pc_geom.llm, &m->m[0][0]);
	M3dMaths_LastVec = geom_mvmva(pc_geom.llm, pc_geom.bk, a.ir[0], a.ir[1], a.ir[2], 12, 0, 0);
}
