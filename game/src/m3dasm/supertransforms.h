/* Body shared by M3dAsm_SetSuperTransforms and M3dAsm_SetSuperTransformsDewibbled
 * (decomp/src/m3dasm/setsupertransforms*.c). */
#ifndef M3DASM_SUPERTRANSFORMS_H
#define M3DASM_SUPERTRANSFORMS_H

#include "m3dmaths.h"

/* Combines a super item's world transforms with one part's animation frame c:
 *   light matrix    = B * Rc
 *   rotation matrix = A * Rc
 *   translation     = A * (Tc >> tshift) + At   (saturated to 16 bits per axis)
 * where A is the world-to-view matrix (rotation + translation) and B the lighting matrix.
 * tshift is 4 for the normal transforms (the frame's translation is in 16ths) and 0 for the
 * dewibbled ones, which keep the 16x scale for precision. */
/* (out of line, as it always was: with both callers in m3dasm.c it got inlined) */
static __inline__ __attribute__((noinline)) void m3d_set_super_transforms(MATRIX *a, MATRIX *b,
                                                                          struct SMatrix *c,
                                                                          int tshift) {
	short prod[3][3];
	int16_t rot[3][3];
	int32_t at[3];
	GeomAcc t;

	geom_load_matrix(rot, &a->m[0][0]);
	geom_load_vector(at, a->t[0], a->t[1], a->t[2]);
	geom_load_matrix(pc_geom.llm, &b->m[0][0]);

	m3d_mul_matrix_v(pc_geom.llm, c->m, prod); /* B * Rc */
	geom_load_matrix(pc_geom.llm, &prod[0][0]);
	m3d_mul_matrix_v(rot, c->m, prod); /* A * Rc */

	/* A * Tc + At (16-bit vertex components, as the hardware takes them) */
	t = geom_mvmva(rot, at, (int16_t)(c->t[0] >> tshift),
	               (int16_t)(c->t[1] >> tshift), (int16_t)(c->t[2] >> tshift), 12, 0, 0);

	geom_load_matrix(pc_geom.rot, &prod[0][0]);
	geom_load_vector(pc_geom.tr, t.ir[0], t.ir[1], t.ir[2]);
}

#endif
