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
	long t[3];
} MATRIX;

typedef struct {
	long vx, vy, vz, pad;
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
	long chx, chy;
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
MATRIX *RotMatrixX(long r, MATRIX *m);
MATRIX *RotMatrixY(long r, MATRIX *m);
MATRIX *RotMatrixZ(long r, MATRIX *m);
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
void SetBackColor(long rbk, long gbk, long bbk);
void SetFarColor(long rfc, long gfc, long bfc);
void SetGeomOffset(long ofx, long ofy);
void SetGeomScreen(long h);
void ReadSZfifo3(long *sz0, long *sz1, long *sz2);
void ReadSZfifo4(long *szx, long *sz0, long *sz1, long *sz2);
void ReadSXSYfifo(long *sxy0, long *sxy1, long *sxy2);
void ReadRGBfifo(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2);
void ReadGeomOffset(long *ofx, long *ofy);
long ReadGeomScreen(void);

/* transformation and lighting */
void TransRot_32(VECTOR *v0, VECTOR *v1, long *flag);
long TransRotPers(SVECTOR *v0, long *sxy, long *p, long *flag);
long TransRotPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2,
                   long *p, long *flag);
void pers_map(int abuf, SVECTOR **vertex, int tex[4][2], u_short *dtext);
void PhongLine(int istart_x, int iend_x, int p, int q, u_short **pixx, int fs, int ft, int i4,
               int det);
long RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag);
long RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2,
                   long *p, long *flag);
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag);
void RotTransSV(SVECTOR *v0, SVECTOR *v1, long *flag);
void LocalLight(SVECTOR *v0, VECTOR *v1);
void LightColor(VECTOR *v0, VECTOR *v1);
void DpqColorLight(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2);
void DpqColor(CVECTOR *v0, long p, CVECTOR *v1);
void DpqColor3(CVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long p, CVECTOR *v3, CVECTOR *v4,
               CVECTOR *v5);
void Intpl(VECTOR *v0, long p, CVECTOR *v1);
VECTOR *Square12(VECTOR *v0, VECTOR *v1);
VECTOR *Square0(VECTOR *v0, VECTOR *v1);
VECTOR *SquareSL12(SVECTOR *v0, VECTOR *v1);
VECTOR *SquareSL0(SVECTOR *v0, VECTOR *v1);
SVECTOR *SquareSS12(SVECTOR *v0, SVECTOR *v1);
SVECTOR *SquareSS0(SVECTOR *v0, SVECTOR *v1);
void NormalColor(SVECTOR *v0, CVECTOR *v1);
void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5);
void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2);
void NormalColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6);
void NormalColorCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2);
void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4,
                     CVECTOR *v5, CVECTOR *v6);
void ColorDpq(VECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2);
void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2);
long NormalClip(long sxy0, long sxy1, long sxy2);
long AverageZ3(long sz0, long sz1, long sz2);
long AverageZ4(long sz0, long sz1, long sz2, long sz3);
void OuterProduct12(VECTOR *v0, VECTOR *v1, VECTOR *v2);
void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *v2);
long Lzc(long data);
long RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1,
                   long *sxy2, long *sxy3, long *p, long *flag);
void RotTransPersN(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *p, u_short *flag, long n);
void RotTransPers3N(SVECTOR *v0, DVECTOR *v1, u_short *sz, u_short *flag, long n);
void RotMeshH(short *Yheight, DVECTOR *Vo, u_short *sz, u_short *flag, short Xoffset,
              short Zoffset, short m, short n, DVECTOR *base);
long RotAverage3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2,
                 long *p, long *flag);
long RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1,
                 long *sxy2, long *sxy3, long *p, long *flag);
long RotNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p,
               long *otz, long *flag);
long RotNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1,
               long *sxy2, long *sxy3, long *p, long *otz, long *flag);
long RotAverageNclip3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2,
                      long *p, long *otz, long *flag);
long RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, long *sxy0, long *sxy1,
                      long *sxy2, long *sxy3, long *p, long *otz, long *flag);
long RotColorDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long *flag);
long RotColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4, SVECTOR *v5,
                  CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2, CVECTOR *v7, CVECTOR *v8,
                  CVECTOR *v9, long *flag);
long RotAverageNclipColorDpq3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                              SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2,
                              CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag);
long RotAverageNclipColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                              SVECTOR *v5, CVECTOR *v6, long *sxy0, long *sxy1, long *sxy2,
                              CVECTOR *v7, CVECTOR *v8, CVECTOR *v9, long *otz, long *flag);
long RotColorMatDpq(SVECTOR *v0, SVECTOR *v1, CVECTOR *v2, long *sxy, CVECTOR *v3, long matc,
                    long flag);
void ColorMatDpq(SVECTOR *v0, CVECTOR *v1, long p, CVECTOR *v2, long matc);
void ColorMatCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2, long matc);
void LoadAverage12(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2);
void LoadAverageShort12(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2);
void LoadAverage0(VECTOR *v0, VECTOR *v1, long p0, long p1, VECTOR *v2);
void LoadAverageShort0(SVECTOR *v0, SVECTOR *v1, long p0, long p1, SVECTOR *v2);
void LoadAverageByte(u_char *v0, u_char *v1, long p0, long p1, u_char *v2);
void LoadAverageCol(u_char *v0, u_char *v1, long p0, long p1, u_char *v2);

/* scalar math */
long VectorNormal(VECTOR *v0, VECTOR *v1);
long VectorNormalS(VECTOR *v0, SVECTOR *v1);
long VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
long SquareRoot0(long a);
long SquareRoot12(long a);
void InvSquareRoot(long a, long *b, long *c);
void gteMIMefunc(SVECTOR *otp, SVECTOR *dfp, long n, long p);
void SetFogFar(long a, long h);
void SetFogNear(long a, long h);
void SetFogNearFar(long a, long b, long h);
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
long ratan2(long y, long x);
long p2otz(long p, long projection);
long otz2p(long otz, long projection);

/* packet (PMD/SMD/RMD) renderers */
void RotPMD_F3(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_G3(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_FT3(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_GT3(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_F4(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_G4(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_FT4(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_GT4(long *pa, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_F3(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_G3(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_FT3(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_GT3(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_F4(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_G4(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_FT4(long *pa, long *va, u_long *ot, int otlen, int id, int backc);
void RotPMD_SV_GT4(long *pa, long *va, u_long *ot, int otlen, int id, int backc);

#define PSYQ_LIBGTE_XMD(name)                                                                      \
	void name(long *pa, u_long *ot, int otlen, int id, int sclip, int hclip, int vclip,          \
	          int nclipmode);
#define PSYQ_LIBGTE_XMD_SV(name)                                                                   \
	void name(long *pa, long *va, u_long *ot, int otlen, int id, int sclip, int hclip, int vclip, \
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
void InitClip(EVECTOR *evbfad, long hw, long vw, long h, long znear, long zfar);
long Clip3F(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, EVECTOR **evmx);
long Clip3FP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, EVECTOR **evmx);
long Clip4F(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, EVECTOR **evmx);
long Clip4FP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, EVECTOR **evmx);
long Clip3FT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
             EVECTOR **evmx);
long Clip3FTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
              EVECTOR **evmx);
long Clip4FT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
             short *uv2, short *uv3, EVECTOR **evmx);
long Clip4FTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
              short *uv2, short *uv3, EVECTOR **evmx);
long Clip3G(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2,
            EVECTOR **evmx);
long Clip3GP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2,
             EVECTOR **evmx);
long Clip4G(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, CVECTOR *rgb0, CVECTOR *rgb1,
            CVECTOR *rgb2, CVECTOR *rgb3, EVECTOR **evmx);
long Clip4GP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, CVECTOR *rgb0, CVECTOR *rgb1,
             CVECTOR *rgb2, CVECTOR *rgb3, EVECTOR **evmx);
long Clip3GT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
             CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, EVECTOR **evmx);
long Clip3GTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, short *uv0, short *uv1, short *uv2,
              CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, EVECTOR **evmx);
long Clip4GT(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
             short *uv2, short *uv3, CVECTOR *rgb0, CVECTOR *rgb1, CVECTOR *rgb2, CVECTOR *rgb3,
             EVECTOR **evmx);
long Clip4GTP(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, short *uv0, short *uv1,
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
long RotColorDpq3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *v4,
                      SVECTOR *v5, CVECTOR *v6);
void NormalColor_nom(SVECTOR *v0);
void NormalColor3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2);
void NormalColorDpq_nom(SVECTOR *v0, CVECTOR *v1, long p);
void NormalColorDpq3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, long p);
void NormalColorCol_nom(SVECTOR *v0, CVECTOR *v1);
void NormalColorCol3_nom(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3);

#ifdef __cplusplus
}
#endif

#endif
