/* M3dMaths_AddScaledLastVec12__FP7SVectori (0x8008b860) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* v + the last vector (lastvec.h) scaled by the 4.12 factor k (16 bits), as the new last
 * vector: ((v << 12) + last * k) >> 12, saturated to 16 bits.
 * test_asm: lastvec ir */
void M3dMaths_AddScaledLastVec12__FP7SVectori(struct SVector *v, int k) {
	int32_t base[3];

	base[0] = v->vx;
	base[1] = v->vy;
	base[2] = v->vz;
	M3dMaths_LastVec = geom_gpl((int16_t)k, M3dMaths_LastVec.ir, base, 12, 0, 0, 0, 0);
}
