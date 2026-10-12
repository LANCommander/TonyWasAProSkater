/* M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX (0x80089eac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Transforms v by the animation frame sm (rotation + short translation), then the result by m
 * (rotation + translation): the GTE does v' = Rsm*v + Tsm (rtv0tr, into IR1..3), then
 * Rm*v' + Tm with m loaded as the light matrix and its translation as the background colour
 * (llirbk). The point is left in the GTE's IR1..3 / MAC1..3 for the caller to read.
 * GTE state changed: V0, rotation matrix + TR (= sm), light matrix (= m), RBK/GBK/BBK (= m->t). */
void M3dAsm_TransformTwice__FP7SVectorP7SMatrixP6MATRIX(struct SVector *v, struct SMatrix *sm,
                                                        MATRIX *m)
{
	PC_GteSetData(0, m3d_pack(v->vx, v->vy));
	PC_GteSetData(1, (uint32_t)(int)v->vz);
	gte_SetRotMatrix(sm);
	gte_ldtr(sm->t[0], sm->t[1], sm->t[2]);
	gte_ldbkdir(m->t[0], m->t[1], m->t[2]);
	gte_rtv0tr();
	gte_SetLightMatrix(m);
	gte_llirbk();
}
