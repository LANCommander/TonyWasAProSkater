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
 * dewibbled ones, which keep the 16x scale for precision.
 * GTE state left behind: V0 = the shifted translation, V1/V2 = columns 1 and 2 of Rc, IR/MAC from
 * the translation product, and the rotation, translation and light registers above. */
static __inline__ void m3d_set_super_transforms(MATRIX *a, MATRIX *b, struct SMatrix *c,
                                                int tshift) {
	short prod[3][3];
	int col;

	gte_SetRotMatrix(a);
	gte_SetTransMatrix(a);
	gte_SetLightMatrix(b);

	/* the columns of the frame's rotation into V0, V1, V2 */
	for (col = 0; col < 3; col++) {
		PC_GteSetData(2 * col, m3d_pack(c->m[0][col], c->m[1][col]));
		PC_GteSetData(2 * col + 1, (uint32_t)(int)c->m[2][col]);
	}

	m3d_mul_matrix_v(M3D_LLV0, prod); /* B * Rc */
	m3d_load_ctrl_matrix(8, prod);
	m3d_mul_matrix_v(M3D_RTV0, prod); /* A * Rc */

	PC_GteSetData(0, m3d_pack(c->t[0] >> tshift, c->t[1] >> tshift));
	PC_GteSetData(1, (uint32_t)(c->t[2] >> tshift));
	gte_rtv0tr(); /* A * Tc + At */

	m3d_load_ctrl_matrix(0, prod);
	gte_ldtr((int)PC_GteGetData(9), (int)PC_GteGetData(10), (int)PC_GteGetData(11));
}

#endif
