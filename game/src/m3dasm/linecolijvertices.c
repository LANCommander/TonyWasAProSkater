/* M3dAsm_LineColijProcessVertices__FP6SModelP16SProjectedVertexiP6VECTOR (0x8008aa50) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
/* test_asm: gte_out none (no caller reads what retail left in the GTE's data registers) */

/* First half of the per-face line test (M3dColij_LineToThisItem): moves every vertex of a model
 * into line space (offset by the item position relative to the line start, then through the
 * colour matrix and translation the caller set up, which turn the line into the +z axis from 0 to
 * length). Each vertex is stored with an outcode: which side of the line it lies on in x and y,
 * and whether it's before the start or past the end in z. Returns the AND of all the outcodes
 * (masked to PV_ALL): non-zero when the whole model is on one side, so the line misses it. */
#include "linecolij.h"

int M3dAsm_LineColijProcessVertices__FP6SModelP16SProjectedVertexiP6VECTOR(SModel *model,
                                                                           SProjectedVertex *out,
                                                                           int length,
                                                                           VECTOR *ofs) {
	SVECTOR *v = model->Vertices;
	int n = model->NumVertices;
	unsigned all = PV_ALL;

	do {
		/* colour matrix * the vertex (16 bits per coordinate) + translation */
		GeomAcc r = geom_mvmva(pc_geom.lcm, pc_geom.tr, (int16_t)(v->vx + ofs->vx),
		                       (int16_t)(v->vy + ofs->vy), (int16_t)(v->vz + ofs->vz), 12, 0, 0);
		int x = r.mac[0], y = r.mac[1], z = r.mac[2];
		unsigned code;

		code = 0;

		if (x < 0)
			code |= PV_XNEG;

		if (y < 0)
			code |= PV_YNEG;

		if (z < 0)
			code |= PV_ZNEG;

		if (length < z)
			code |= PV_ZFAR;

		if (x > 0)
			code |= PV_XPOS;

		if (y > 0)
			code |= PV_YPOS;

		out->x = (short)x;
		out->y = (short)y;
		out->z = (short)z;
		out->Outcode = (unsigned short)code;
		all &= code;
		out++;
		v++;
	} while (--n);

	return (int)all;
}
