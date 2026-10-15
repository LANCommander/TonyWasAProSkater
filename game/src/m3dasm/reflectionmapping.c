/* M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi (0x800866cc) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */
#include "vertex.h"
#include "game/scratchpad.h"

extern uint32_t RefMapU0V0; /* the reflection map's texture-page origin, added to every uv */

/* The state this function borrows, as retail saved the GTE's control registers in the
 * scratchpad (the same words; the port works on a copy and leaves the state alone). */
struct SavedGte {
	uint32_t tr[3];  /* 0x00: translation */
	uint32_t of[3];  /* 0x0c: screen offset x, y, distance (sign-extended) */
	uint32_t llm[5]; /* 0x18: light matrix, packed pairs */
	uint32_t rt[5];  /* 0x2c: rotation, packed pairs */
};

#define SAVED ((struct SavedGte *)SCRATCHPAD)

/* a matrix as five words of pairs of 16-bit elements, the last one sign-extended */
static void pack_matrix(uint32_t w[5], const int16_t m[3][3]) {
	w[0] = vtx_pack(m[0][0], m[0][1]);
	w[1] = vtx_pack(m[0][2], m[1][0]);
	w[2] = vtx_pack(m[1][1], m[1][2]);
	w[3] = vtx_pack(m[2][0], m[2][1]);
	w[4] = (uint32_t)(int32_t)m[2][2];
}

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
 * world: the model-to-world rotation; pos: the model's position relative to the camera, in world
 * space; the model's vertices follow its 28-byte header, then one normal per vertex.
 *
 * The light matrix is used as scratch: first the transposed (inverse) current rotation, column 1
 * scaled by 5/8, takes -pos into model space (the eye position); then its first row holds the
 * eye vector and the rest the identity, so one product gives e.n along with the normal. A slip in
 * the retail code (`or t4,t4,t4` where `or t4,t4,t5` was meant) drops the reflected y: the
 * vector projected is (r.x - e.x, -e.y, r.z - e.z) with the eye vector's halves subtracted as one
 * packed word. That is what the game shows, so it is kept.
 * test_asm: gte_out none */
void M3dAsm_GetReflectionMapping__FP6MATRIXPlPUi(MATRIX *world, int *pos, uint32_t *model) {
	struct SavedGte *s = SAVED;
	GeomState g = pc_geom;
	const int16_t(*r)[3] = pc_geom.rot;
	uint32_t eye_xy, uv0;
	int eye_z, n, i;
	const uint32_t *v, *nrm;
	char *pv, *pool, *top;
	GeomAcc a;

	for (i = 0; i < 3; i++)
		s->tr[i] = (uint32_t)pc_geom.tr[i];

	s->of[0] = (uint32_t)pc_geom.ofx;
	s->of[1] = (uint32_t)pc_geom.ofy;
	s->of[2] = (uint32_t)(int32_t)(int16_t)pc_geom.h;
	pack_matrix(s->llm, pc_geom.llm);
	pack_matrix(s->rt, pc_geom.rot);

	geom_load_vector(g.tr, 0, 0, 0);
	g.ofx = 0;
	g.ofy = 0;
	g.h = 1024;

	/* light matrix = transpose of the rotation, its first column scaled by 5/8 */
	g.llm[0][0] = (int16_t)scale_5_8(r[0][0]);
	g.llm[0][1] = r[1][0];
	g.llm[0][2] = r[2][0];
	g.llm[1][0] = (int16_t)scale_5_8(r[0][1]);
	g.llm[1][1] = r[1][1];
	g.llm[1][2] = r[2][1];
	g.llm[2][0] = (int16_t)scale_5_8(r[0][2]);
	g.llm[2][1] = r[1][2];
	g.llm[2][2] = r[2][2];

	geom_load_matrix(g.rot, &world->m[0][0]);

	/* the eye in model space */
	uv0 = RefMapU0V0;
	a = geom_mvmva(g.llm, 0, (int16_t)-pos[0], (int16_t)-pos[1], (int16_t)-pos[2], 12, 0, 0);

	n = ((short *)model)[1]; /* NumVertices */
	v = model + 7;           /* the vertices, after the 28-byte header */
	nrm = v + 2 * n;
	eye_xy = vtx_pack(a.mac[0], a.mac[1]);
	eye_z = a.mac[2];

	/* rows 2 and 3 of the light matrix: the identity (row 1 is set below) */
	memset(g.llm, 0, sizeof g.llm);
	g.llm[1][1] = 4096;
	g.llm[2][2] = 4096;

	pv = (char *)ProjectedVertices;
	pool = (char *)FreeStitchVertex;
	top = pv + STITCH_TOP;

	/* retail counts n down to zero after each vertex: models always have vertices */
	do {
		uint32_t flags = v[1] >> 16, uv, e_xy, sxy, w;
		int e_z;

		if (flags & VF_STITCH) {
			*(uint32_t *)(pv + 3204) = *(uint32_t *)(top - v[0] + 3204);
		} else {
			int16_t k[3];

			/* the eye vector: eye - vertex (x and y as one packed word) */
			e_xy = eye_xy - v[0];
			e_z = (eye_z - (int)v[1]) & 0xffff;
			g.llm[0][0] = (int16_t)e_xy;
			g.llm[0][1] = (int16_t)(e_xy >> 16);
			g.llm[0][2] = (int16_t)e_z;
			a = geom_mvmva(g.llm, 0, (short)nrm[0], (int32_t)nrm[0] >> 16, (short)nrm[1], 12,
			               0, 0); /* e.n, n.y, n.z */

			/* 2 (e.n) n */
			k[0] = (short)nrm[0];
			k[1] = a.ir[1];
			k[2] = a.ir[2];
			a = geom_gpf((int16_t)(a.ir[0] << 1), k, 12, 0, 0, 0, 0);
			w = ((uint32_t)a.ir[0] & 0xffff) - e_xy;
			sxy = geom_rtps(&g, (int16_t)w, (int16_t)(w >> 16),
			                (int16_t)((uint32_t)a.ir[2] - (uint32_t)e_z), 12, 0, 0, 0).sxy;

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
}
