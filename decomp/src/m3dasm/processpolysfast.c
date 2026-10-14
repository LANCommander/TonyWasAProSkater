/* M3dAsm_ProcessPolysFast__FPUiP7SVECTORi (0x800898ac) -- C interpretation of hand-written assembly (M3DASM) */
/* Not byte-matching by design: listed in reimpl.tsv, verified against the retail MIPS by
 * platform/pc/tests/test_asm.c. */

/* The faces of a model the fast transforms put in the scratchpad cache (vertex.h, SFastVertex):
 * as M3dAsm_ProcessPolys (processpolys.h) without GlobalFaceFlags, lighting, reflection mapping,
 * subdivision or water. The screen words carry their clip flags, which the GPU ignores, so they
 * go into the packets as they are. Faces with a corner beyond Yon (bit 31 clear) or all corners
 * off one screen side are dropped. Includes FullFast (0x80089d20) and LinktwinFast (0x80089d54).
 *
 * test_asm: kseg 0x800d1a54
 * (pPoly: the retail code stores it back without its top byte; the port keeps it in KSEG0.)
 * test_asm: colourcode
 *
 * When the packet buffer is full retail jumps to ProcessPolys' exit (0x80087628), which stores
 * pPoly and returns but restores registers from the wrong (40-byte) frame; this returns
 * normally. */
#include "processpolys.h"

void M3dAsm_ProcessPolysFast__FPUiP7SVECTORi(uint32_t *faces, SVECTOR *normals, int count) {
	PolyState ps;
	const uint32_t *face = faces;
	uint32_t *colours = pColourTable;

	(void)normals;

	if (count == 0)
		return;

	pp_begin(&ps);

	for (; count != 0; count--, face = (const uint32_t *)((const char *)face + (face[0] >> 16))) {
		uint32_t vi = face[FACE_VERTS], f, xy[4], c, len, *p;
		int z[4], n1, n2, i, visible, tri;
		struct SFastVertex *v[4];

		for (i = 0; i < 4; i++) {
			v[i] = &FAST_CACHE[vi >> 8 * i & 0xff];
			xy[i] = v[i]->XY;
		}

		f = face[FACE_FLAGS];

		if ((int32_t)(xy[0] & xy[1] & xy[2] & xy[3]) >= 0 ||
		    (xy[0] & xy[1] & xy[2] & xy[3] & 0x6000e000))
			continue;

		if (!(f & FF_DRAW))
			continue;

		/* the windings of corners 0, 1, 2 and 1, 2, 3 (the words x4: the flags shift out of
		 * the 16-bit halves) */
		n1 = pp_nclip(xy[0] << 2, xy[1] << 2, xy[2] << 2);
		n2 = pp_nclip(xy[1] << 2, xy[2] << 2, xy[3] << 2);

		if (!SCR16(SCR_MIRROR))
			visible = n1 > 0 || n2 < 0;
		else
			visible = n1 < 0 || n2 > 0;

		if (!visible)
			continue;

		p = PKT(ps.pkt);
		p[2] = xy[0];

		if (ps.pkt >= ps.limit)
			break; /* FullFast */

		ps.face = face;
		ps.flags = f;

		for (i = 0; i < 4; i++)
			z[i] = v[i]->z;

		ps.zmax = z[0];

		for (i = 1; i < 4; i++)
			if (ps.zmax < z[i])
				ps.zmax = z[i];

		pp_fog(&ps);

		f = ps.flags;
		tri = (f & FF_TRIANGLE) != 0;

		c = face[FACE_COLOUR];

		if (!(f & FF_GOURAUD)) {
			uint32_t rgb = pp_dpcs(c, c, ps.fog);

			M3dAsm_ColourCode = c;

			if (!(f & FF_TEXTURED)) {
				p[3] = xy[1];
				p[4] = xy[2];

				if (tri) {
					pp_code(&ps, rgb, CODE_F3);

					len = 4;
				} else {
					p[5] = xy[3];
					pp_code(&ps, rgb, CODE_F4);

					len = 5;
				}
			} else {
				p[4] = xy[1];
				p[6] = xy[2];
				p[3] = face[FACE_UV0];
				p[5] = face[FACE_UV1] | (f & FF_ABR) << 14;
				p[7] = face[FACE_UV23];

				if (tri) {
					pp_code(&ps, rgb, CODE_FT3);

					len = 7;
				} else {
					p[8] = xy[3];
					p[9] = face[FACE_UV23] >> 16;
					pp_code(&ps, rgb, CODE_FT4);

					len = 9;
				}
			}
		} else {
			/* gouraud: the table colours of the corners depth-cued, corners 0-2 with the code byte
			 * the colour code holds (the last face's), corner 3 with its own (also for a
			 * triangle) */
			uint32_t in[3], col[3], col0, col3;

			in[0] = colours[c & 0xff];
			in[1] = colours[c >> 8 & 0xff];
			in[2] = colours[c >> 16 & 0xff];
			geom_dpct(&pc_geom, in, (uint8_t)(M3dAsm_ColourCode >> 24), (int16_t)ps.fog, 12, 0,
			          col, 0);
			M3dAsm_ColourCode = colours[c >> 24];
			col3 = pp_dpcs(M3dAsm_ColourCode, M3dAsm_ColourCode, ps.fog);

			if (!(f & FF_TEXTURED)) {
				p[4] = xy[1];
				p[6] = xy[2];

				col0 = col[0];
				p[3] = col[1];
				p[5] = col[2];

				if (tri) {
					pp_code(&ps, col0, CODE_G3);

					len = 6;
				} else {
					p[8] = xy[3];
					p[7] = col3;
					pp_code(&ps, col0, CODE_G4);

					len = 8;
				}
			} else {
				uint32_t uv23 = face[FACE_UV23];

				p[5] = xy[1];
				p[8] = xy[2];
				p[3] = face[FACE_UV0];
				p[6] = face[FACE_UV1] | (f & FF_ABR) << 14;
				p[9] = uv23;

				col0 = col[0] & 0xffffff;
				p[4] = col[1];
				p[7] = col[2];

				if (tri) {
					pp_code(&ps, col0, CODE_GT3);

					len = 9;
				} else {
					p[11] = xy[3];
					p[12] = uv23 >> 16;
					p[10] = col3;
					pp_code(&ps, col0, CODE_GT4);

					len = 12;
				}
			}
		}

		pp_link(&ps, ps.pkt, ps.pkt, len, xy);
	}

	pp_end(&ps);
}
