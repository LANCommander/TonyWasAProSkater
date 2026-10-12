/* M3dMaths_CrossWithLastVec12__FP7SVector (0x8008b7b8) -- C interpretation of hand-written assembly (M3DMATHS) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "m3dmaths.h"

/* Cross product of v with the "last vector" in the GTE's IR1..3, in 4.12 (OP, sf=1): the GTE
 * takes the first operand from the rotation matrix diagonal, so v goes into R11, R22, R33
 * (R12 and R23 become 0, the rest of the rotation matrix is left as it was). Result in
 * IR1..3/MAC1..3: (v.y*IR3 - v.z*IR2, v.z*IR1 - v.x*IR3, v.x*IR2 - v.y*IR1) >> 12. */
void M3dMaths_CrossWithLastVec12__FP7SVector(struct SVector *v)
{
	PC_GteSetCtrl(0, (unsigned short)v->vx); /* R11 = x, R12 = 0 */
	PC_GteSetCtrl(2, (unsigned short)v->vy); /* R22 = y, R23 = 0 */
	PC_GteSetCtrl(4, (unsigned short)v->vz); /* R33 = z */
	gte_op12();
}
