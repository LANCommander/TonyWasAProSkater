/* M3dAsm_LineColijProcessVertices__FP6SModelP16SProjectedVertexiP6VECTOR (0x8008aa50) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* First half of the per-face line test (M3dColij_LineToThisItem): moves every vertex of a model
 * into line space (offset by the item position relative to the line start, then through the GTE
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
		int x, y, z;
		unsigned code;

		PC_GteSetData(9, (unsigned)(v->vx + ofs->vx));
		PC_GteSetData(10, (unsigned)(v->vy + ofs->vy));
		PC_GteSetData(11, (unsigned)(v->vz + ofs->vz));
		gte_mvmva(1, 2, 3, 0, 0); /* colour matrix * IR + translation */

		x = GTE_S(25);
		y = GTE_S(26);
		z = GTE_S(27);

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
