/* M3dAsm_OutcodeVerticesInWater (0x80086500) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
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
 * below the water. The GTE's second rotation row becomes the camera transform's second row (with
 * TRY = -water level), so MVMVA of a view-space vertex gives its height above the water (IR2).
 * Outcode bits 0x80/0x8000 are then toggled for the renderer's water clipping: those set in
 * `mask` (WaterClipValue) for a vertex above the water, the others for one below it. RT13 (it
 * shares a word with RT21) and the second row are left changed. */
void M3dAsm_OutcodeVerticesInWater(int n, int mask) {
	struct SProjectedVertex *pv = ProjectedVertices;
	struct SRotatedVertex *rv = RotatedVertices;
	const uint32_t *row = (const uint32_t *)&pCurrentCamera->Transform.m[0][2];

	gte_setc(GTE_RT + 1, row[0]); /* RT13, RT21 */
	gte_setc(GTE_RT + 2, row[1]); /* RT22, RT23 */
	gte_setc(GTE_TRY, -WaterNormal.pad);

	/* retail counts n down to zero after each vertex: n == 0 is never passed */
	do {
		uint32_t below;

		gte_set(GTE_VXY0, rv->xy);
		gte_set(GTE_VZ0, ((uint32_t *)pv)[1]);
		gte_mvmva(1, MX_RT, MV_V0, CV_TR, 0);

		below = (uint32_t)gte_get(GTE_IR2) >> 16; /* 0xffff if negative */
		pv->Outcode ^= (ushort)((below ^ (uint32_t)mask) & 0x8080);
		pv++;
		rv++;
	} while (--n != 0);
}
