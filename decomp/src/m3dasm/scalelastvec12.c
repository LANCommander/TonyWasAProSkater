/* M3dMaths_ScaleLastVec12__Fi (0x8008b844) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Multiplies the "last vector" in the GTE's IR1..3 by the 4.12 factor k (GPF, sf=1:
 * IR = IR * k >> 12, k goes through IR0). */
void M3dMaths_ScaleLastVec12__Fi(int k) {
	PC_GteSetData(8, (uint32_t)k);
	gte_gpf12();
}
