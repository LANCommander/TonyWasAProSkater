/* M3dAsm_SetTransVector__FP6Vector (0x80089f54) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Loads a 20.12 fixed-point position into the GTE translation vector (TRX/TRY/TRZ) as whole
 * units (arithmetic shift right by 12).
 *
 * The retail block also holds the label M3dAsm_NewSetRotTransMatrix__FP7SMatrix (0x80089f80),
 * which nothing calls; SetTransVector returns before it and does not fall through. */
void M3dAsm_SetTransVector__FP6Vector(struct Vector *v) {
	gte_ldtr(v->vx >> 12, v->vy >> 12, v->vz >> 12);
}
