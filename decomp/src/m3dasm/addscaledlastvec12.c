/* M3dMaths_AddScaledLastVec12__FP7SVectori (0x8008b860) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* v + the "last vector" (IR1..3) scaled by the 4.12 factor k (GPL, sf=1):
 * MAC = ((v << 12) + IR * k) >> 12, saturated into IR1..3. v goes in through MAC1..3, k
 * through IR0. */
void M3dMaths_AddScaledLastVec12__FP7SVectori(struct SVector *v, int k)
{
	PC_GteSetData(8, (uint32_t)k);
	PC_GteSetData(25, (uint32_t)(int)v->vx);
	PC_GteSetData(26, (uint32_t)(int)v->vy);
	PC_GteSetData(27, (uint32_t)(int)v->vz);
	gte_gpl12();
}
