/* M3dMaths_ScaleLastVec12__Fi (0x8008b844) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* Multiplies the last vector (lastvec.h) by the 4.12 factor k (16 bits): v = v * k >> 12.
 * test_asm: lastvec ir */
void M3dMaths_ScaleLastVec12__Fi(int k) {
	M3dMaths_LastVec = geom_gpf((int16_t)k, M3dMaths_LastVec.ir, 12, 0, 0, 0, 0);
}
