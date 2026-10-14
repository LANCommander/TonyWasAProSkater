/* M3dAsm_SetTransVector__FP6Vector (0x80089f54) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* A 20.12 fixed-point position becomes the translation, as whole units (arithmetic shift right
 * by 12).
 *
 * The retail block also holds the label M3dAsm_NewSetRotTransMatrix__FP7SMatrix (0x80089f80),
 * which nothing calls; SetTransVector returns before it and does not fall through. */
void M3dAsm_SetTransVector__FP6Vector(struct Vector *v) {
	geom_load_vector(pc_geom.tr, v->vx >> 12, v->vy >> 12, v->vz >> 12);
}
