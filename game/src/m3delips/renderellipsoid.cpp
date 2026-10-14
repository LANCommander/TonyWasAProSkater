/* M3dElips_RenderEllipsoid__FP6SModelP6MATRIXUiiiP7Texture (0x8007e300) */
/* C++ (M3DELIPS.CPP). Locals, their order and types come from the demo's MAIN.SYM. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

#include "../m3dasm/lastvec.h"
#include "game/camera.h"

#define Box ((short *)&pModel->Box)

extern SCamera *pCurrentCamera;
extern SVECTOR *ProjectedVertices;
extern P_TAG *CurrentOrderingTable;
extern char *pPoly;
extern char *PolyBufferEnd;

void M3dMaths_ZeroMat(ushort *);
void M3dMaths_CopyMat(ushort *, ushort *);
void M3dMaths_TransposeMat(ushort *, ushort *);
void M3dMaths_TransformLastVec(ushort *);
int M3dMaths_LastVecMagnitude(void);
void M3dMaths_CrossWithLastVec12(SVector *);
void M3dMaths_NormaliseLastVec(void);
void M3dMaths_ScaleLastVec12(int);

void M3dElips_RenderEllipsoid(SModel *pModel, MATRIX *pTransform, uint Rgb, int Scale,
                              int OTPushback, Texture *pTex) {
	short a, b, c, d;
	short Mag_d_m;
	int d12;
	int k;
	short R[3][3], RT[3][3], d_Ainv[3][3];
	SVector l, t, v, d_m, Scaled_q, e, f, Vertex, Centre;
	CVector d_m_Squared;
	POLY_FT4 *pQuad;
	uint OTPos;

	a = ((Box[0] - Box[1]) >> 1) * Scale >> 12;
	b = ((Box[2] - Box[3]) >> 1) * Scale >> 12;
	c = ((Box[4] - Box[5]) >> 1) * Scale >> 12;
	Centre.vx = (Box[0] + Box[1]) >> 1;
	Centre.vy = (Box[2] + Box[3]) >> 1;
	Centre.vz = (Box[4] + Box[5]) >> 1;

	/* M3DMATHS' last vector (lastvec.h) carries the vector between its routines */
	M3dMaths_SetLastVec(&Centre);
	M3dMaths_TransformLastVec((ushort *)pTransform);
	M3dMaths_GetLastVec(&t);

	t.vx += pTransform->t[0];
	t.vy += pTransform->t[1];
	t.vz += pTransform->t[2];
	t.vx -= pCurrentCamera->Position.vx;
	t.vy -= pCurrentCamera->Position.vy;
	t.vz -= pCurrentCamera->Position.vz;
	M3dMaths_SetLastVec(&t);
	M3dMaths_NormaliseLastVec();
	M3dMaths_GetLastVec(&l);

	d = (a + b + c) / 3;
	M3dMaths_CopyMat((ushort *)pTransform, (ushort *)R);
	M3dMaths_TransposeMat((ushort *)R, (ushort *)RT);
	M3dMaths_ZeroMat((ushort *)d_Ainv);

	d12 = d << 12;
	d_Ainv[0][0] = d12 / a;
	d_Ainv[1][1] = d12 / b;
	d_Ainv[2][2] = d12 / c;

	M3dMaths_SetLastVec(&l);
	M3dMaths_TransformLastVec((ushort *)RT);
	M3dMaths_NormaliseLastVec();
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_GetLastVec(&d_m);

	Mag_d_m = M3dMaths_LastVecMagnitude();
	M3dMaths_GetLastVecSums(&d_m_Squared);

	v.vx = v.vy = v.vz = 0;

	if (d_m_Squared.vx <= d_m_Squared.vy && d_m_Squared.vx <= d_m_Squared.vz)
		v.vx = 4096;
	else if (d_m_Squared.vy <= d_m_Squared.vx && d_m_Squared.vy <= d_m_Squared.vz)
		v.vy = 4096;
	else
		v.vz = 4096;

	M3dMaths_SetLastVec(&d_m);
	M3dMaths_CrossWithLastVec12(&v);
	M3dMaths_GetLastVec(&Scaled_q);
	M3dMaths_NormaliseLastVec();

	k = a * b / d * c * Mag_d_m / d >> 12;
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_TransformLastVec((ushort *)R);
	M3dMaths_CrossWithLastVec12(&l);
	M3dMaths_ScaleLastVec12(k);
	M3dMaths_GetLastVec(&e);

	M3dMaths_SetLastVec(&d_m);
	M3dMaths_CrossWithLastVec12(&Scaled_q);
	M3dMaths_NormaliseLastVec();
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_TransformLastVec((ushort *)R);
	M3dMaths_CrossWithLastVec12(&l);
	M3dMaths_ScaleLastVec12(k);
	M3dMaths_GetLastVec(&f);

	geom_ld_mat(pc_geom.rot, &pCurrentCamera->View);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	{
		/* the quad's corners, t -+ e -+ f (16 bits each), projected; the sort depth is the
		 * average of the four */
		static const int se[4] = { -1, 1, -1, 1 }, sf[4] = { -1, -1, 1, 1 };
		int16_t q[4][3];
		GeomProj p[4];
		int i;

		for (i = 0; i < 4; i++) {
			q[i][0] = (int16_t)(t.vx + se[i] * e.vx + sf[i] * f.vx);
			q[i][1] = (int16_t)(t.vy + se[i] * e.vy + sf[i] * f.vy);
			q[i][2] = (int16_t)(t.vz + se[i] * e.vz + sf[i] * f.vz);
		}

		geom_rtpt(&pc_geom, q, 12, 0, p, 0, 0);
		p[3] = geom_rtps(&pc_geom, q[3][0], q[3][1], q[3][2], 12, 0, 0, 0);

		for (i = 0; i < 4; i++)
			*(uint32_t *)&ProjectedVertices[i] = p[i].sxy;

		OTPos = geom_avsz4(&pc_geom, p[0].sz, p[1].sz, p[2].sz, p[3].sz, 0, 0);
	}

	OTPos += OTPushback;

	if (OTPos >= 4096)
		OTPos = 4095;

	pQuad = (POLY_FT4 *)pPoly;
	pPoly += sizeof(POLY_FT4);

	if (pPoly > PolyBufferEnd) {
		pPoly = (char *)pQuad;
		return;
	}

	*(uint *)&pQuad->r0 = Rgb;
	setPolyFT4(pQuad);
	setSemiTrans(pQuad, 1);

	{
		SVECTOR *sc = ProjectedVertices;

		pQuad->x0 = sc[0].vx;
		pQuad->y0 = sc[0].vy;
		pQuad->x1 = sc[1].vx;
		pQuad->y1 = sc[1].vy;
		pQuad->x2 = sc[2].vx;
		pQuad->y2 = sc[2].vy;
		pQuad->x3 = sc[3].vx;
		pQuad->y3 = sc[3].vy;
	}

	pQuad->u0 = pTex->u0;
	pQuad->v0 = pTex->v0;
	pQuad->u1 = pTex->u1 - 1;
	pQuad->v1 = pTex->v1;
	pQuad->u2 = pTex->u2;
	pQuad->v2 = pTex->v2 - 1;
	pQuad->u3 = pTex->u3 - 1;
	pQuad->v3 = pTex->v3 - 1;
	pQuad->clut = pTex->clut;
	pQuad->tpage = (pTex->tpage & ~0x60) | 0x20;

	{
		addPrim(&CurrentOrderingTable[OTPos], pQuad);
	}
}
