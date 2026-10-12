/* M3dElips_RenderShadow__FP6SModelP6MATRIXP8CSVectorP7CVectorP7SVECTORG7CVECTOR (0x8007d9ac) flags=-fstructure-noalias */
/* C++ (M3DELIPS.CPP). Locals, their order and types come from the demo's MAIN.SYM. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include <inline_c.h>
}
#include "game/m3d.h"
#include "game/camera.h"

#define Box ((short *)&pModel->Box)

/* V0/V1/V2 from registers, one mtc2 statement each (the demo's line table has packed xy and z
   on separate lines) */
#define gte_ldVXY0(r) __asm__ volatile("mtc2 %0, $0" : : "r"(r))
#define gte_ldVZ0(r)  __asm__ volatile("mtc2 %0, $1" : : "r"(r))
#define gte_ldVXY1(r) __asm__ volatile("mtc2 %0, $2" : : "r"(r))
#define gte_ldVZ1(r)  __asm__ volatile("mtc2 %0, $3" : : "r"(r))
#define gte_ldVXY2(r) __asm__ volatile("mtc2 %0, $4" : : "r"(r))
#define gte_ldVZ2(r)  __asm__ volatile("mtc2 %0, $5" : : "r"(r))

extern SCamera *pCurrentCamera;
extern SVECTOR *ProjectedVertices;
extern u_long *CurrentOrderingTable;
extern char *pPoly;
extern char *PolyBufferEnd;
extern Texture *pShadowTexture;

int M3dElips_MinDotProd;

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

	gte_ldsv(&Centre);
	M3dMaths_TransformLastVec((ushort *)pTransform);
	gte_stsv(&t);

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

	gte_ldsv(pLightVec);
	M3dMaths_TransformLastVec((ushort *)RT);
	M3dMaths_NormaliseLastVec();
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	gte_stsv(&d_m);

	Mag_d_m = M3dMaths_LastVecMagnitude();
	gte_stlvnl(&d_m_Squared);

	v.vx = v.vy = v.vz = 0;

	if (d_m_Squared.vx <= d_m_Squared.vy && d_m_Squared.vx <= d_m_Squared.vz)
		v.vx = 4096;
	else if (d_m_Squared.vy <= d_m_Squared.vx && d_m_Squared.vy <= d_m_Squared.vz)
		v.vy = 4096;
	else
		v.vz = 4096;

	gte_ldsv(&d_m);
	M3dMaths_CrossWithLastVec12(&v);
	gte_stsv(&Scaled_q);
	M3dMaths_NormaliseLastVec();

	k = a * b / d * c * Mag_d_m / d >> 12;
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_TransformLastVec((ushort *)R);
	M3dMaths_CrossWithLastVec12((SVector *)pLightVec);
	M3dMaths_ScaleLastVec12(k);
	gte_stsv(&e);

	gte_ldsv(&d_m);
	M3dMaths_CrossWithLastVec12(&Scaled_q);
	M3dMaths_NormaliseLastVec();
	M3dMaths_TransformLastVec((ushort *)d_Ainv);
	M3dMaths_TransformLastVec((ushort *)R);
	M3dMaths_CrossWithLastVec12((SVector *)pLightVec);
	M3dMaths_ScaleLastVec12(k);
	gte_stsv(&f);

	gte_ldv0(pNormal);
	M3dMaths_TransformVec0((ushort *)pLightVec);
	gte_stlvnl0(&n_dot_l);

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
	M3dMaths_TransformVec0((ushort *)M);
	gte_stsv(&DotProds);

	if (DotProds.vx > 0)
		return;

	t.vx -= pCurrentCamera->Position.vx;
	t.vy -= pCurrentCamera->Position.vy;
	t.vz -= pCurrentCamera->Position.vz;
	gte_ldsv(pLightVec);
	M3dMaths_AddScaledLastVec12(&t, (DotProds.vx << 12) / n_dot_l);
	gte_stsv(&Centre);
	gte_ldsv(pLightVec);
	M3dMaths_AddScaledLastVec12(&e, -(DotProds.vy << 12) / n_dot_l);
	gte_stsv(&e_Proj);
	gte_ldsv(pLightVec);
	M3dMaths_AddScaledLastVec12(&f, -(DotProds.vz << 12) / n_dot_l);
	gte_stsv(&f_Proj);

	gte_SetRotMatrix(&pCurrentCamera->View);
	__asm__ volatile("ctc2 $0, $5; ctc2 $0, $6; ctc2 $0, $7");

	t1 = Centre.vx - e_Proj.vx - f_Proj.vx;
	t2 = Centre.vy - e_Proj.vy - f_Proj.vy;
	t3 = Centre.vz - e_Proj.vz - f_Proj.vz;
	gte_ldVXY0((t2 << 16) + (t1 & 0xffff));
	gte_ldVZ0(t3);

	t1 = Centre.vx + e_Proj.vx - f_Proj.vx;
	t2 = Centre.vy + e_Proj.vy - f_Proj.vy;
	t3 = Centre.vz + e_Proj.vz - f_Proj.vz;
	gte_ldVXY1((t2 << 16) + (t1 & 0xffff));
	gte_ldVZ1(t3);

	t1 = Centre.vx - e_Proj.vx + f_Proj.vx;
	t2 = Centre.vy - e_Proj.vy + f_Proj.vy;
	t3 = Centre.vz - e_Proj.vz + f_Proj.vz;
	gte_ldVXY2((t2 << 16) + (t1 & 0xffff));
	gte_ldVZ2(t3);
	gte_rtpt();

	t1 = Centre.vx + e_Proj.vx + f_Proj.vx;
	t2 = Centre.vy + e_Proj.vy + f_Proj.vy;
	t3 = Centre.vz + e_Proj.vz + f_Proj.vz;
	gte_stsxy0(&ProjectedVertices[0]);
	gte_stsxy1(&ProjectedVertices[1]);
	gte_stsxy2(&ProjectedVertices[2]);
	gte_ldVXY0((t2 << 16) + (t1 & 0xffff));
	gte_ldVZ0(t3);
	gte_rtps();
	gte_stsxy(&ProjectedVertices[3]);
	gte_avsz4();
	gte_stotz(&OTPos);

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
		SVECTOR *sc = ProjectedVertices;

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
		u_long *ot = (u_long *)(OTPos * 4 + (uint)CurrentOrderingTable);

		addPrim(ot, pQuad);
	}
}
