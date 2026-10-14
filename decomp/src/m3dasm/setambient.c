/* M3dAsm_SetAmbient__FPi (0x80086f20) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "m3dmaths.h"

/* The ambient light colour (three ints: r, g, b) becomes the background colour. */
void M3dAsm_SetAmbient__FPi(int *rgb) {
	geom_load_vector(pc_geom.bk, rgb[0], rgb[1], rgb[2]);
}
