/* M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi (0x800866cc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"

extern uint32_t RefMapU0V0; /* the reflection map's texture-page origin, added to every uv */

/* The GTE state this function borrows, saved in the scratchpad and put back at the end. */
struct SavedGte {
	uint32_t tr[3];  /* 0x00: TRX, TRY, TRZ */
	uint32_t of[3];  /* 0x0c: OFX, OFY, H */
	uint32_t llm[5]; /* 0x18: light matrix */
	uint32_t rt[5];  /* 0x2c: rotation matrix */
};

#define SAVED ((struct SavedGte *)0x1f800000)

/* 2560/4096 of a 4.12 matrix element */
static int scale_5_8(int m) {
	return (2560 * m) >> 12;
}

/* Reflection-map texture coordinates for a model's vertices (environment mapping): for each
 * vertex, the eye vector (eye - vertex, in model space) is reflected about the vertex normal,
 * r = 2 (e.n) n - e, and r is projected with the world transform onto a screen of focal length
 * 1024 centred on 0; its screen x, y give u, v (64 steps across) relative to RefMapU0V0. The uv
 * word goes to the vertex's RotatedVertices Special word (ProjectedVertices + 3204) and to the
 * stitch pool for exported vertices; stitch vertices copy theirs from the pool.
 *
 * world: the model-to-world rotation (loaded as the GTE rotation); pos: the model's position
 * relative to the camera, in world space; the model's vertices follow its 28-byte header, then
 * one normal per vertex.
 *
 * The light matrix is used as scratch: first the transposed (inverse) current rotation, column 1
 * scaled by 5/8, takes -pos into model space (the eye position); then its first row holds the
 * eye vector and the rest the identity, so one MVMVA gives e.n along with the normal. A slip in
 * the retail code (`or t4,t4,t4` where `or t4,t4,t5` was meant) drops the reflected y: the
 * vector projected is (r.x - e.x, -e.y, r.z - e.z) with the eye vector's halves subtracted as one
 * packed word. That is what the game shows, so it is kept. */
void M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi(MATRIX *world, long *pos, uint32_t *model) {
	struct SavedGte *s = SAVED;
	uint32_t r0, r1, r2, r3, eye_xy, uv0;
	int eye_z, n, i;
	const uint32_t *v, *nrm;
	char *pv, *pool, *top;

	for (i = 0; i < 3; i++)
		s->tr[i] = gte_getc(GTE_TRX + i);

	s->of[0] = gte_getc(GTE_OFX);
	s->of[1] = gte_getc(GTE_OFY);
	s->of[2] = gte_getc(GTE_H);
	gte_ldtr(0, 0, 0);
	gte_setc(GTE_OFX, 0);
	gte_setc(GTE_OFY, 0);
	gte_setc(GTE_H, 1024);

	for (i = 0; i < 5; i++)
		s->llm[i] = gte_getc(GTE_LLM + i);

	for (i = 0; i < 5; i++)
		s->rt[i] = gte_getc(GTE_RT + i);

	/* light matrix = transpose of the rotation, its first column scaled by 5/8 */
	r0 = s->rt[0];                                                     /* R11 R12 */
	r1 = s->rt[1];                                                     /* R13 R21 */
	r2 = s->rt[2];                                                     /* R22 R23 */
	r3 = s->rt[3];                                                     /* R31 R32 */
	gte_setc(GTE_LLM + 0, vtx_pack(scale_5_8((short)r0), r1 >> 16));   /* L11 L12 */
	gte_setc(GTE_LLM + 1, vtx_pack(r3, scale_5_8((int32_t)r0 >> 16))); /* L13 L21 */
	gte_setc(GTE_LLM + 2, (r2 & 0xffff) | (r3 & 0xffff0000u));         /* L22 L23 */
	gte_setc(GTE_LLM + 3, vtx_pack(scale_5_8((short)r1), r2 >> 16));   /* L31 L32 */
	gte_setc(GTE_LLM + 4, s->rt[4]);                                   /* L33 */

	gte_SetRotMatrix(world);

	/* the eye in model space */
	gte_set(GTE_IR1, -pos[0]);
	gte_set(GTE_IR2, -pos[1]);
	gte_set(GTE_IR3, -pos[2]);

	uv0 = RefMapU0V0;
	gte_mvmva(1, MX_LLM, MV_IR, CV_NONE, 0);

	n = ((short *)model)[1]; /* NumVertices */
	v = model + 7;           /* the vertices, after the 28-byte header */
	nrm = v + 2 * n;
	eye_xy = vtx_pack(gte_get(GTE_MAC1), gte_get(GTE_MAC2));
	eye_z = gte_get(GTE_MAC3);

	/* rows 2 and 3 of the light matrix: the identity (L21 is set with row 1 below) */
	gte_setc(GTE_LLM + 2, 4096);
	gte_setc(GTE_LLM + 3, 0);
	gte_setc(GTE_LLM + 4, 4096);

	pv = (char *)ProjectedVertices;
	pool = (char *)FreeStitchVertex;
	top = pv + STITCH_TOP;

	/* retail counts n down to zero after each vertex: models always have vertices */
	do {
		uint32_t flags = v[1] >> 16, uv, e_xy, sxy;
		int e_z, dot2;

		if (flags & VF_STITCH) {
			*(uint32_t *)(pv + 3204) = *(uint32_t *)(top - v[0] + 3204);
		} else {
			/* the eye vector: eye - vertex (x and y as one packed word) */
			e_xy = eye_xy - v[0];
			e_z = (eye_z - (int)v[1]) & 0xffff;
			gte_setc(GTE_LLM + 0, e_xy); /* L11 L12 */
			gte_setc(GTE_LLM + 1, e_z);  /* L13, L21 = 0 */
			gte_set(GTE_VXY0, nrm[0]);
			gte_set(GTE_VZ0, nrm[1]);
			gte_mvmva(1, MX_LLM, MV_V0, CV_NONE, 0); /* IR1 = e.n, IR2/IR3 = n.y, n.z */

			dot2 = gte_get(GTE_IR1) << 1;
			gte_set(GTE_IR1, nrm[0]); /* n.x */
			gte_set(GTE_IR0, dot2);
			gte_gpf12(); /* 2 (e.n) n */
			gte_set(GTE_VXY0, ((uint32_t)gte_get(GTE_IR1) & 0xffff) - e_xy);
			gte_set(GTE_VZ0, (uint32_t)gte_get(GTE_IR3) - (uint32_t)e_z);
			gte_rtps();

			sxy = PC_GteGetData(GTE_SXY2);
			uv = (((sxy >> 5) + 32) & 0x3f) | (((sxy >> 13) + 0x2000) & 0x3f00);
			uv += uv0;
			*(uint32_t *)(pv + 3204) = uv;

			if (flags & VF_EXPORT) {
				*(uint32_t *)(pool + 3204) = uv;
				pool -= 8;
			}
		}

		v += 2;
		nrm += 2;
		pv += 8;
	} while (--n != 0);

	for (i = 0; i < 3; i++)
		gte_setc(GTE_TRX + i, s->tr[i]);

	gte_setc(GTE_OFX, s->of[0]);
	gte_setc(GTE_OFY, s->of[1]);
	gte_setc(GTE_H, s->of[2]);

	for (i = 0; i < 5; i++)
		gte_setc(GTE_LLM + i, s->llm[i]);

	for (i = 0; i < 5; i++)
		gte_setc(GTE_RT + i, s->rt[i]);
}
