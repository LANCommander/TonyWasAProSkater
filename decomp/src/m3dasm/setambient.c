/* M3dAsm_SetAmbient__FPi (0x80086f20) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Loads the ambient light colour (three ints: r, g, b) into the GTE background colour
 * registers RBK/GBK/BBK. */
void M3dAsm_SetAmbient__FPi(int *rgb) {
	gte_ldbkdir(rgb[0], rgb[1], rgb[2]);
}
