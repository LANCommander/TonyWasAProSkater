/* M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi (0x800862d4) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

/* The fast version of TransformAndOutcodeSuperVertices: screen words with folded-in clip flags
 * and depths go to the scratchpad cache (no rotated positions, no outcodes), stitched as usual.
 * Returns the AND of the screen words' flag bits 0x60006000 (0x60000000 for no vertices). */
int M3dAsm_TransformAndOutcodeSuperVerticesFast__FP7SVECTORi(SVECTOR *v, int n) {
	return vtx_transform_super_fast((const uint32_t *)v, n, 0);
}
