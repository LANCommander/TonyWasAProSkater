/* M3DELIPS.CPP: drawing a super item's part ellipsoids (textured) and its shadows (RenderSuperItem
   calls them per part). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3delips.h"
#include "game/m3d.h"
#include "game/camera.h"
#include "game/bit.h"
#include "game/globals.h"
#include "game/m3dasm.h"
#include "m3dasm/lastvec.h"

#define Box ((short *)&pModel->Box)
extern int M3dElips_MinDotProd;
void M3dMaths_ZeroMat(ushort *);
void M3dMaths_CopyMat(ushort *, ushort *);
void M3dMaths_TransposeMat(ushort *, ushort *);
void M3dMaths_TransformLastVec(ushort *);
void M3dMaths_TransformVec0(ushort *);
int M3dMaths_LastVecMagnitude(void);
void M3dMaths_CrossWithLastVec12(SVector *);
void M3dMaths_NormaliseLastVec(void);
void M3dMaths_ScaleLastVec12(int);
void M3dMaths_AddScaledLastVec12(SVector *, int);

/* M3dElips_RenderShadow__FP6SModelP6MATRIXP8CSVectorP7CVectorP7SVECTORG7CVECTOR (0x8007d9ac) */
/* Locals, their order and types come from the demo's MAIN.SYM. */
void M3dElips_RenderShadow(SModel *pModel, MATRIX *pTransform, CSVector *pNormal, CVector *pPoint,
                           SVECTOR *pLightVec, CVECTOR rgb) {
	short a, b, c, d;
	short Mag_d_m;
	int d12;
	int k;
	int n_dot_l;
	short R[3][3], RT[3][3], d_Ainv[3][3], M[3][3];
	SVector t, v, d_m, Scaled_q, e, f, DotProds, Centre, e_Proj, f_Proj, Vertex;
	CVector d_m_Squared;
	POLY_FT4 *pQuad;
	uint OTPos;
	int t1, t2, t3;

	Centre.vx = (Box[0] + Box[1]) >> 1;
	Centre.vy = (Box[2] + Box[3]) >> 1;
	Centre.vz = (Box[4] + Box[5]) >> 1;
	a = ((Box[0] - Box[1]) >> 1) * 5 >> 2;
	b = ((Box[2] - Box[3]) >> 1) * 11 >> 3;
	c = ((Box[4] - Box[5]) >> 1) * 5 >> 2;

	/* M3DMATHS' last vector (lastvec.h) carries the vector between its routines */
	M3dMaths_SetLastVec(&Centre);
	M3dMaths_TransformLastVec((ushort *)pTransform);
	M3dMaths_GetLastVec(&t);

	t.vx += pTransform->t[0];
	t.vy += pTransform->t[1];
	t.vz += pTransform->t[2];

	d = (a + b + c) / 3;
	M3dMaths_CopyMat((ushort *)pTransform, (ushort *)R);
	M3dMaths_TransposeMat((ushort *)R, (ushort *)RT);
	M3dMaths_ZeroMat((ushort *)d_Ainv);

	d12 = d << 12;
	d_Ainv[0][0] = d12 / a;
	d_Ainv[1][1] = d12 / b;
	d_Ainv[2][2] = d12 / c;

	M3dMaths_SetLastVec(pLightVec);
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
	M3dMaths_CrossWithLastVec12((SVector *)pLightVec);
	M3dMaths_ScaleLastVec12(k);
	M3dMaths_GetLastVec(&e);

	M3dMaths_SetLastVec(&d_m);
	M3dMaths_CrossWithLastVec12(&Scaled_q);
	M3dMaths_NormaliseLastVec();
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_TransformLastVec((ushort *)R);
	M3dMaths_CrossWithLastVec12((SVector *)pLightVec);
	M3dMaths_ScaleLastVec12(k);
	M3dMaths_GetLastVec(&f);

	M3dMaths_SetLastVec(pNormal); /* (retail: V0) */
	M3dMaths_TransformVec0((ushort *)pLightVec);
	n_dot_l = M3dMaths_LastVec.mac[0];

	if (n_dot_l < M3dElips_MinDotProd)
		return;

	M[0][0] = (pPoint->vx >> 12) - t.vx;
	M[0][1] = (pPoint->vy >> 12) - t.vy;
	M[0][2] = (pPoint->vz >> 12) - t.vz;
	M[1][0] = e.vx;
	M[1][1] = e.vy;
	M[1][2] = e.vz;
	M[2][0] = f.vx;
	M[2][1] = f.vy;
	M[2][2] = f.vz;
	M3dMaths_SetLastVec(pNormal);
	M3dMaths_TransformVec0((ushort *)M);
	M3dMaths_GetLastVec(&DotProds);

	if (DotProds.vx > 0)
		return;

	t.vx -= pCurrentCamera->Position.vx;
	t.vy -= pCurrentCamera->Position.vy;
	t.vz -= pCurrentCamera->Position.vz;
	M3dMaths_SetLastVec(pLightVec);
	M3dMaths_AddScaledLastVec12(&t, (DotProds.vx << 12) / n_dot_l);
	M3dMaths_GetLastVec(&Centre);
	M3dMaths_SetLastVec(pLightVec);
	M3dMaths_AddScaledLastVec12(&e, -(DotProds.vy << 12) / n_dot_l);
	M3dMaths_GetLastVec(&e_Proj);
	M3dMaths_SetLastVec(pLightVec);
	M3dMaths_AddScaledLastVec12(&f, -(DotProds.vz << 12) / n_dot_l);
	M3dMaths_GetLastVec(&f_Proj);

	geom_ld_mat(pc_geom.rot, &pCurrentCamera->View);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	{
		/* the quad's corners, Centre -+ e_Proj -+ f_Proj (16 bits each), projected; the sort
		 * depth is the average of the four */
		static const int se[4] = { -1, 1, -1, 1 }, sf[4] = { -1, -1, 1, 1 };
		int16_t q[4][3];
		GeomProj p[4];
		int i;

		for (i = 0; i < 4; i++) {
			q[i][0] = (int16_t)(Centre.vx + se[i] * e_Proj.vx + sf[i] * f_Proj.vx);
			q[i][1] = (int16_t)(Centre.vy + se[i] * e_Proj.vy + sf[i] * f_Proj.vy);
			q[i][2] = (int16_t)(Centre.vz + se[i] * e_Proj.vz + sf[i] * f_Proj.vz);
		}

		geom_rtpt(&pc_geom, q, 12, 0, p, 0, 0);
		p[3] = geom_rtps(&pc_geom, q[3][0], q[3][1], q[3][2], 12, 0, 0, 0);

		for (i = 0; i < 4; i++)
			*(uint32_t *)&((SVECTOR *)ProjectedVertices)[i] = p[i].sxy;

		OTPos = geom_avsz4(&pc_geom, p[0].sz, p[1].sz, p[2].sz, p[3].sz, 0, 0);
	}

	if (OTPos >= 4096)
		OTPos = 4095;

	pQuad = (POLY_FT4 *)pPoly;
	pPoly += sizeof(POLY_FT4);

	if (pPoly > PolyBufferEnd) {
		pPoly = (char *)pQuad;
		return;
	}

	*(u_long *)&pQuad->r0 = (*(u_long *)&rgb & 0xffffff) | 0x2e000000;
	setlen(pQuad, 9);

	{
		uint t0, t1, t2, t3;
		SVECTOR *sc = (SVECTOR *)ProjectedVertices;

		t0 = *(uint *)&sc[0];
		t1 = *(uint *)&sc[1];
		t2 = *(uint *)&sc[2];
		t3 = *(uint *)&sc[3];
		*(uint *)&pQuad->x0 = t0;
		*(uint *)&pQuad->x1 = t1;
		*(uint *)&pQuad->x2 = t2;
		*(uint *)&pQuad->x3 = t3;
	}

	pQuad->u0 = pShadowTexture->u0;
	pQuad->v0 = pShadowTexture->v0;
	pQuad->u1 = pShadowTexture->u1 - 1;
	pQuad->v1 = pShadowTexture->v1;
	pQuad->u2 = pShadowTexture->u2;
	pQuad->v2 = pShadowTexture->v2 - 1;
	pQuad->u3 = pShadowTexture->u3 - 1;
	pQuad->v3 = pShadowTexture->v3 - 1;

	{
		Texture *tx = pShadowTexture;

		pQuad->clut = tx->clut;
		pQuad->tpage = (tx->tpage & ~0x60) | 0x40;
	}

	{
		addPrim(&CurrentOrderingTable[OTPos], pQuad);
	}
}

/* M3dElips_RenderEllipsoid__FP6SModelP6MATRIXUiiiP7Texture (0x8007e300) */
/* Locals, their order and types come from the demo's MAIN.SYM. */
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
			*(uint32_t *)&((SVECTOR *)ProjectedVertices)[i] = p[i].sxy;

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
		SVECTOR *sc = (SVECTOR *)ProjectedVertices;

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
