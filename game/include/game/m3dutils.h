/* M3DUTILS.H: M3D helpers: hook positions on animated super items, animation in-betweening, angles
   from a matrix, closest points of two lines and vector normalising (M3DUTILS.CPP). */
#ifndef GAME_M3DUTILS_H
#define GAME_M3DUTILS_H

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"
#include "game/vector.h"
#include "game/m3d.h"

class CSuper;

/* functions */
void M3dUtils_ReadHooksPacket(CSuper *s, void *p);
void M3dUtils_GetHookPosition(VECTOR *pResult, CSuper *pSuper, int Hook);
void M3dUtils_InBetween(CSuper *s);
void M3dUtils_AnglesFromMatrix(SVECTOR *out, MATRIX *m);
int M3dUtils_LineLineClosestPoints(CVector *L1Start, CVector *L1End, CVector *L2Start, CVector *L2End, CVector *PointL1, int *t1, CVector *PointL2, int *t2);
void M3dUtils_NormaliseVector(CVector *v);
void FUN_80082054();

/* globals */
extern SMatrix TweenBuffer[];

#endif
