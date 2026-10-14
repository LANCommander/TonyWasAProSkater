/* M3dMaths_CrossWithLastVec12__FP7SVector (0x8008b7b8) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"
#include "lastvec.h"

/* Cross product of v with the last vector (lastvec.h), in 4.12, as the new last vector:
 * (v.y*l.z - v.z*l.y, v.z*l.x - v.x*l.z, v.x*l.y - v.y*l.x) >> 12. Retail fed v to the GTE through
 * the rotation matrix's diagonal, which the state keeps: rot[0][0], [1][1], [2][2] = v,
 * rot[0][1] = rot[1][2] = 0.
 * test_asm: lastvec ir */
void M3dMaths_CrossWithLastVec12__FP7SVector(struct SVector *v) {
	int16_t d[3];

	geom_load_op_diag(&pc_geom, (unsigned short)v->vx, (unsigned short)v->vy,
	                  (unsigned short)v->vz);
	geom_op_diag(&pc_geom, d);
	M3dMaths_LastVec = geom_op(d, M3dMaths_LastVec.ir, 12, 0, 0);
}
