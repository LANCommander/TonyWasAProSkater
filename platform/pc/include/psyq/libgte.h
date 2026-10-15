/* Psy-Q libgte types and entry points for the PC build. The structures keep their PS1 layouts
 * (the game data depends on them); the functions the game uses are src/pc_libgte.c. */
#ifndef PSYQ_LIBGTE_H
#define PSYQ_LIBGTE_H

#include <sys/types.h>

#define ONE 4096 /* 1.0 in GTE fixed point */
#define TMPVTXNUM 16
#define OUTVTXNUM 10

typedef struct {
	short m[3][3];
	int t[3];
} MATRIX;

typedef struct {
	int vx, vy, vz, pad;
} VECTOR;

typedef struct {
	short vx, vy, vz, pad;
} SVECTOR;

typedef struct {
	u_char r, g, b, cd;
} CVECTOR;

typedef struct {
	short vx, vy;
} DVECTOR;

typedef struct {
	SVECTOR v;
	VECTOR sxyz;
	DVECTOR sxy;
	CVECTOR rgb;
	short txuv, pad;
	int chx, chy;
} EVECTOR;

typedef struct {
	SVECTOR v;
	u_char uv[2];
	u_short pad;
	CVECTOR c;
	DVECTOR sxy;
	u_long sz;
} RVECTOR;

typedef struct {
	RVECTOR r01, r12, r20;
	RVECTOR *r0, *r1, *r2;
	u_long *rtn;
} CRVECTOR3;

typedef struct {
	u_long ndiv;
	u_long pih, piv;
	u_short clut, tpage;
	CVECTOR rgbc;
	u_long *ot;
	RVECTOR r0, r1, r2;
	CRVECTOR3 cr[5];
} DIVPOLYGON3;

typedef struct {
	RVECTOR r01, r02, r31, r32, rc;
	RVECTOR *r0, *r1, *r2, *r3;
	u_long *rtn;
} CRVECTOR4;

typedef struct {
	u_long ndiv;
	u_long pih, piv;
	u_short clut, tpage;
	CVECTOR rgbc;
	u_long *ot;
	RVECTOR r0, r1, r2, r3;
	CRVECTOR4 cr[5];
} DIVPOLYGON4;

typedef struct {
	short xy[3];
	short uv[2];
	short rgb[3];
} SPOL;

typedef struct {
	short sxy[4][2];
	short sz[4][2];
	short uv[4][2];
	short rgb[4][3];
	short code;
} POL4;

typedef struct {
	short sxy[3][2];
	short sz[3][2];
	short uv[3][2];
	short rgb[3][3];
	short code;
} POL3;

typedef struct {
	SVECTOR *v;
	SVECTOR *n;
	SVECTOR *u;
	CVECTOR *c;
	u_long len;
} TMESH;

typedef struct {
	SVECTOR *v;
	SVECTOR *n;
	SVECTOR *u;
	CVECTOR *c;
	u_long lenv;
	u_long lenh;
} QMESH;

#ifdef __cplusplus
extern "C" {
#endif

/* setup and matrices */
void InitGeom(void);
void EigenMatrix(MATRIX *m, MATRIX *t);
int IsIdMatrix(MATRIX *m);
MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2);
MATRIX *MulRotMatrix0(MATRIX *m0, MATRIX *m1);
MATRIX *MulMatrix(MATRIX *m0, MATRIX *m1);
MATRIX *MulMatrix2(MATRIX *m0, MATRIX *m1);
MATRIX *MulRotMatrix(MATRIX *m0);
MATRIX *SetMulMatrix(MATRIX *m0, MATRIX *m1);
MATRIX *SetMulRotMatrix(MATRIX *m0);
VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1);
VECTOR *ApplyRotMatrix(SVECTOR *v0, VECTOR *v1);
VECTOR *ApplyRotMatrixLV(VECTOR *v0, VECTOR *v1);
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
SVECTOR *ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1);
VECTOR *ApplyTransposeMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixXZY(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixYZX(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixZXY(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixZYX(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrix_gte(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixYXZ_gte(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixZYX_gte(SVECTOR *r, MATRIX *m);
MATRIX *RotMatrixX(int r, MATRIX *m);
MATRIX *RotMatrixY(int r, MATRIX *m);
MATRIX *RotMatrixZ(int r, MATRIX *m);
MATRIX *RotMatrixC(SVECTOR *r, MATRIX *m);
MATRIX *TransMatrix(MATRIX *m, VECTOR *v);
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v);
MATRIX *ScaleMatrixL(MATRIX *m, VECTOR *v);
MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1);
MATRIX *CompMatrix(MATRIX *m0, MATRIX *m1, MATRIX *m2);
MATRIX *CompMatrixLV(MATRIX *m0, MATRIX *m1, MATRIX *m2);
void MatrixNormal(MATRIX *m, MATRIX *n);
void MatrixNormal_0(MATRIX *m, MATRIX *n);
void MatrixNormal_1(MATRIX *m, MATRIX *n);
void MatrixNormal_2(MATRIX *m, MATRIX *n);

/* GTE register state */
void SetRotMatrix(MATRIX *m);
void SetLightMatrix(MATRIX *m);
void SetColorMatrix(MATRIX *m);
void SetTransMatrix(MATRIX *m);
void PushMatrix(void);
void PopMatrix(void);
void ReadRotMatrix(MATRIX *m);
void ReadLightMatrix(MATRIX *m);
void ReadColorMatrix(MATRIX *m);
void SetRGBcd(CVECTOR *v);
void SetBackColor(int rbk, int gbk, int bbk);
void SetFarColor(int rfc, int gfc, int bfc);
void SetGeomOffset(int ofx, int ofy);
void SetGeomScreen(int h);
void ReadSZfifo3(int *sz0, int *sz1, int *sz2);
void ReadSZfifo4(int *szx, int *sz0, int *sz1, int *sz2);
void ReadSXSYfifo(int *sxy0, int *sxy1, int *sxy2);
void ReadRGBfifo(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2);
void ReadGeomOffset(int *ofx, int *ofy);
int ReadGeomScreen(void);

/* transformation and lighting */
void TransRot_32(VECTOR *v0, VECTOR *v1, int *flag);
int TransRotPers(SVECTOR *v0, int *sxy, int *p, int *flag);
int TransRotPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, int *sxy0, int *sxy1, int *sxy2,
                   int *p, int *flag);
void pers_map(int abuf, SVECTOR **vertex, int tex[4][2], u_short *dtext);
void PhongLine(int istart_x, int iend_x, int p, int q, u_short **pixx, int fs, int ft, int i4,
               int det);
int RotTransPers(SVECTOR *v0, int *sxy, int *p, int *flag);
int RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, int *sxy0, int *sxy1, int *sxy2,
                   int *p, int *flag);
void RotTrans(SVECTOR *v0, VECTOR *v1, int *flag);
void RotTransSV(SVECTOR *v0, SVECTOR *v1, int *flag);
void LocalLight(SVECTOR *v0, VECTOR *v1);
void LightColor(VECTOR *v0, VECTOR *v1);
void DpqColorLight(VECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2);
void DpqColor(CVECTOR *v0, int p, CVECTOR *v1);
void DpqColor3(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2, int p, CVECTOR *v3, CVECTOR *v4,
               CVECTOR *v5);
void Intpl(VECTOR *v0, int p, CVECTOR *v1);
VECTOR *Square12(VECTOR *v0, VECTOR *v1);
VECTOR *Square0(VECTOR *v0, VECTOR *v1);
VECTOR *SquareSL12(SVECTOR *v0, VECTOR *v1);
VECTOR *SquareSL0(SVECTOR *v0, VECTOR *v1);
SVECTOR *SquareSS12(SVECTOR *v0, SVECTOR *v1);
SVECTOR *SquareSS0(SVECTOR *v0, SVECTOR *v1);
void NormalColor(SVECTOR *v0, CVECTOR *v1);
void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5);
void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2);
void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, int p, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6);
void NormalColorCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2);
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6);
void ColorDpq(VECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2);
void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2);
int NormalClip(int sxy0, int sxy1, int sxy2);
int AverageZ3(int sz0, int sz1, int sz2);
int AverageZ4(int sz0, int sz1, int sz2, int sz3);
void OuterProduct12(VECTOR *v0, VECTOR *v1, VECTOR *v2);
void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *v2);
int Lzc(int data);
int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, int *sxy0, int *sxy1,
                   int *sxy2, int *sxy3, int *p, int *flag);
void RotTransPersN(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *p, u_short *flag, int n);
void RotTransPers3N(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *flag, int n);
void RotMeshH(short *Yheight, DVECTOR *Vo, u_short *sz, u_short *flag, short Xoffset,
              short Zoffset, short m, short n, DVECTOR *base);
int RotAverage3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, int *sxy0, int *sxy1, int *sxy2,
                 int *p, int *flag);
int RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, int *sxy0, int *sxy1,
                 int *sxy2, int *sxy3, int *p, int *flag);
int RotNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, int *sxy0, int *sxy1, int *sxy2, int *p,
               int *otz, int *flag);
int RotNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, int *sxy0, int *sxy1,
               int *sxy2, int *sxy3, int *p, int *otz, int *flag);
int RotAverageNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, int *sxy0, int *sxy1, int *sxy2,
                      int *p, int *otz, int *flag);
int RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, int *sxy0, int *sxy1,
                      int *sxy2, int *sxy3, int *p, int *otz, int *flag);
int RotColorDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, int *sxy, CVECTOR *v3, int *flag);
int RotColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5,
                  CVECTOR *v6, int *sxy0, int *sxy1, int *sxy2, CVECTOR *v7, CVECTOR *v8,
                  CVECTOR *v9, int *flag);
int RotAverageNclipColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                              SVECTOR *v5, CVECTOR *v6, int *sxy0, int *sxy1, int *sxy2,
                              CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, int *otz, int *flag);
int RotAverageNclipColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                              SVECTOR *v5, CVECTOR *v6, int *sxy0, int *sxy1, int *sxy2,
                              CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, int *otz, int *flag);
int RotColorMatDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, int *sxy, CVECTOR *v3, int matc,
                    int flag);
void ColorMatDpq(SVECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2, int matc);
void ColorMatCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2, int matc);
void LoadAverage12(VECTOR *v0, VECTOR *v1, int p0, int p1, VECTOR *v2);
void LoadAverageShort12(SVECTOR *v0, SVECTOR *v1, int p0, int p1, SVECTOR *v2);
void LoadAverage0(VECTOR *v0, VECTOR *v1, int p0, int p1, VECTOR *v2);
void LoadAverageShort0(SVECTOR *v0, SVECTOR *v1, int p0, int p1, SVECTOR *v2);
void LoadAverageByte(u_char *v0, u_char *v1, int p0, int p1, u_char *v2);
void LoadAverageCol(u_char *v0, u_char *v1, int p0, int p1, u_char *v2);

/* scalar math */
int VectorNormal(VECTOR *v0, VECTOR *v1);
int VectorNormalS(VECTOR *v0, SVECTOR *v1);
int VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
int SquareRoot0(int a);
int SquareRoot12(int a);
void InvSquareRoot(int a, int *b, int *c);
void gteMIMefunc(SVECTOR *otp, SVECTOR *dfp, int n, int p);
void SetFogFar(int a, int h);
void SetFogNear(int a, int h);
void SetFogNearFar(int a, int b, int h);
void SubPol4(POL4 *p, SPOL *sp, int ndiv);
void SubPol3(POL3 *p, SPOL *sp, int ndiv);
int rcos(int a);
int rsin(int a);
/* These share names with C99 <complex.h> builtins; the build should use -fno-builtin. */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif
int ccos(int a);
int csin(int a);
int cln(int a);
int csqrt(int a);
int catan(int a);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
int ratan2(int y, int x);
int p2otz(int p, int projection);
int otz2p(int otz, int projection);

/* packet (PMD/SMD/RMD) renderers */
void RotPMD_F3(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_G3(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_FT3(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_GT3(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_F4(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_G4(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_FT4(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_GT4(int *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_F3(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_G3(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_FT3(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_GT3(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_F4(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_G4(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_FT4(int *pa, int *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_GT4(int *pa, int *va, u_long *ot, int otlen, int id, int backc);

#define PSYQ_LIBGTE_XMD(name)                                                                      \
	void name(int *pa, u_long *ot, int otlen, int id, int sclip, int hclip, int vclip,          \
	          int nclipmode);
#define PSYQ_LIBGTE_XMD_SV(name)                                                                   \
	void name(int *pa, int *va, u_long *ot, int otlen, int id, int sclip, int hclip, int vclip, \
	          int nclipmode);
PSYQ_LIBGTE_XMD(RotSMD_F3)
PSYQ_LIBGTE_XMD(RotSMD_G3)
PSYQ_LIBGTE_XMD(RotSMD_FT3)
PSYQ_LIBGTE_XMD(RotSMD_GT3)
PSYQ_LIBGTE_XMD(RotSMD_F4)
PSYQ_LIBGTE_XMD(RotSMD_G4)
PSYQ_LIBGTE_XMD(RotSMD_FT4)
PSYQ_LIBGTE_XMD(RotSMD_GT4)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_F3)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_G3)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_FT3)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_GT3)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_F4)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_G4)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_FT4)
PSYQ_LIBGTE_XMD_SV(RotSMD_SV_GT4)
PSYQ_LIBGTE_XMD(RotRMD_F3)
PSYQ_LIBGTE_XMD(RotRMD_G3)
PSYQ_LIBGTE_XMD(RotRMD_FT3)
PSYQ_LIBGTE_XMD(RotRMD_GT3)
PSYQ_LIBGTE_XMD(RotRMD_F4)
PSYQ_LIBGTE_XMD(RotRMD_G4)
PSYQ_LIBGTE_XMD(RotRMD_FT4)
PSYQ_LIBGTE_XMD(RotRMD_GT4)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_F3)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_G3)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_FT3)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_GT3)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_F4)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_G4)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_FT4)
PSYQ_LIBGTE_XMD_SV(RotRMD_SV_GT4)
#undef PSYQ_LIBGTE_XMD
#undef PSYQ_LIBGTE_XMD_SV

/* polygon clipping */
void InitClip(EVECTOR *evbfad, int hw, int vw, int h, int znear, int zfar);
int Clip3F(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, EVECTOR **evmx);
int Clip3FP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, EVECTOR **evmx);
int Clip4F(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, EVECTOR **evmx);
int Clip4FP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, EVECTOR **evmx);
int Clip3FT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
             EVECTOR **evmx);
int Clip3FTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
              EVECTOR **evmx);
int Clip4FT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
             short *uv2, short *uv3, EVECTOR **evmx);
int Clip4FTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
              short *uv2, short *uv3, EVECTOR **evmx);
int Clip3G(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2,
            EVECTOR **evmx);
int Clip3GP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2,
             EVECTOR **evmx);
int Clip4G(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, CVECTOR *rgb0, CVECTOR *rgb1,
            CVECTOR *rgb2, CVECTOR *rgb3, EVECTOR **evmx);
int Clip4GP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, CVECTOR *rgb0, CVECTOR *rgb1,
             CVECTOR *rgb2, CVECTOR *rgb3, EVECTOR **evmx);
int Clip3GT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
             CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, EVECTOR **evmx);
int Clip3GTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
              CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, EVECTOR **evmx);
int Clip4GT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
             short *uv2, short *uv3, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, CVECTOR *rgb3,
             EVECTOR **evmx);
int Clip4GTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
              short *uv2, short *uv3, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, CVECTOR *rgb3,
              EVECTOR **evmx);

/* no-result (_nom) forms: results stay in the GTE registers */
void RotTransPers_nom(SVECTOR *v0);
void RotTransPers3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void RotTransPers4_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3);
void RotTrans_nom(SVECTOR *v0);
void RotAverage3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void RotNclip3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void RotAverageNclip3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void RotAverageNclipColorDpq3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                                  SVECTOR *v4, SVECTOR *v5, CVECTOR *v6);
void RotAverageNclipColorCol3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                                  SVECTOR *v4, SVECTOR *v5, CVECTOR *v6);
void RotColorDpq_nom(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2);
int RotColorDpq3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                      SVECTOR *v5, CVECTOR *v6);
void NormalColor_nom(SVECTOR *v0);
void NormalColor3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void NormalColorDpq_nom(SVECTOR *v0, CVECTOR *v1, int p);
void NormalColorDpq3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, int p);
void NormalColorCol_nom(SVECTOR *v0, CVECTOR *v1);
void NormalColorCol3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3);

#ifdef __cplusplus
}
#endif

#endif
