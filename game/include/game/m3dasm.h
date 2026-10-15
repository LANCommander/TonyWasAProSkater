/* The hand-written assembly entry points (M3DMATHS.S / M3DASM.S, past the C++ game code) that C++
   sources call. Names and signatures are the demo SYM's; their C interpretations for the port are
   src/m3dasm/*.c (reimpl.tsv). BoundingSpherePreprocessing and CompactOTagR are plain assembler
   labels, so C linkage. */
#ifndef GAME_M3DASM_H
#define GAME_M3DASM_H

/* CPPPSX doesn't define __cplusplus, so the SDK headers' own extern "C" guards are inactive under
   egcs: wrap them, as the TUs do */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/types.h"
struct SViewport;

struct Item;
struct SMatrix;
struct SModel;
struct SProjectedVertex;
struct SLineInfo;
struct Vector;

extern "C" {
	void M3dAsm_BoundingSpherePreprocessing(Item *pItem, int skip); /* 0x800858ac */
	void CompactOTagR(struct P_TAG *ot, int n); /* 0x80089ddc */
}

void M3dAsm_GetReflectionMapping(MATRIX *world, int *pos, uint *model);   /* 0x800866cc */
void M3dAsm_GetDynamicLighting(SVECTOR *normals, int n);                   /* 0x80086a4c */
void M3dAsm_SetAmbient(int *rgb);                                          /* 0x80086f20 */
void M3dAsm_SetTransVector(Vector *v);                                     /* 0x80089f54 */
void M3dAsm_SetSuperTransformsDewibbled(MATRIX *a, MATRIX *b, SMatrix *c); /* 0x80089fc8 */
void M3dAsm_SetSuperTransforms(MATRIX *a, MATRIX *b, SMatrix *c);          /* 0x8008a1c8 */
int M3dAsm_LineColijProcessVertices(SModel *model, SProjectedVertex *out, int n,
                                    VECTOR *pos); /* 0x8008aa50 */
void M3dAsm_LineColijProcessFaces(SModel *model, SProjectedVertex *pv, SLineInfo *info,
                                  Item *item);        /* 0x8008ab28 */
void M3dMaths_SetIdentityRotation(MATRIX *m);         /* 0x8008b238 */
void M3dMaths_MulRotMatrix(MATRIX *src, MATRIX *dst); /* 0x8008b254 */
void M3dMaths_MulLcmMatrix(MATRIX *src, MATRIX *dst); /* 0x8008b314 */
void M3dMaths_RotMatrixYXZ(SVECTOR *r, MATRIX *m);    /* 0x8008b3d4 */
void M3dMaths_ScaleMatrix(Item *item, MATRIX *m);     /* 0x8008b4e0 */
int M3dMaths_SquareRoot0(int n);                      /* 0x8008b5dc */
int M3dMaths_MulDiv64(int a, int b, int c);           /* 0x8008b62c */
void M3dMaths_TransformLastVec(ushort *m);            /* 0x8008b71c */

/* globals */
extern P_TAG *CurrentOrderingTable;
extern uint M3dColij_OneMask;
extern uint M3dColij_ZeroMask;
extern SViewport *pCurrentViewport;
extern SProjectedVertex *ProjectedVertices;
#endif
