/* M3dAsm_OutcodeVerticesInWater (0x80086500) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */
#include "vertex.h"

struct SCamera {
	uint32_t Style;   /* 0x00 */
	VECTOR Position;  /* 0x04 */
	VECTOR Focus;     /* 0x14 */
	int Distance;     /* 0x24 */
	int Height;       /* 0x28 */
	SVECTOR Angles;   /* 0x2c */
	MATRIX Transform; /* 0x34 */
};

extern struct SCamera *pCurrentCamera;
extern SVECTOR WaterNormal; /* .pad: the water level */

/* After a transform has filled ProjectedVertices/RotatedVertices for n vertices: marks which are
 * below the water. The second row of the rotation becomes the camera transform's second row (with
 * the translation's y = -water level), so a view-space vertex through it gives its height above
 * the water. Outcode bits 0x80/0x8000 are then toggled for the renderer's water clipping: those
 * set in `mask` (WaterClipValue) for a vertex above the water, the others for one below it. Left
 * changed, as retail leaves them: rot[0][2] (retail loads it with rot[1][0]), the second row and
 * the translation's y. */
void M3dAsm_OutcodeVerticesInWater(int n, int mask) {
	struct SProjectedVertex *pv = ProjectedVertices;
	struct SRotatedVertex *rv = RotatedVertices;
	MATRIX *t = &pCurrentCamera->Transform;

	pc_geom.rot[0][2] = t->m[0][2];
	pc_geom.rot[1][0] = t->m[1][0];
	pc_geom.rot[1][1] = t->m[1][1];
	pc_geom.rot[1][2] = t->m[1][2];
	pc_geom.tr[1] = -WaterNormal.pad;

	/* retail counts n down to zero after each vertex: n == 0 is never passed */
	do {
		GeomAcc h = geom_mvmva(pc_geom.rot, pc_geom.tr, (short)rv->xy, (int32_t)rv->xy >> 16,
		                       pv->z, 12, 0, 0);
		uint32_t below;

		below = (uint32_t)(int32_t)h.ir[1] >> 16; /* 0xffff if negative */
		pv->Outcode ^= (ushort)((below ^ (uint32_t)mask) & 0x8080);
		pv++;
		rv++;
	} while (--n != 0);
}
