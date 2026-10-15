/* M3d_BuildTransform__FP6CSuper (0x80085130) */
/* C++. A super item's transform (mTransform): a skater's rotation matrix with the y column
   negated (mExtraFlags bit 0), else its angles' rotation; scaled if the item is (flag 0x200);
   translated to its position in model units. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/bruce.h"
#include "game/m3d.h"

extern "C" void FUN_8008b68c(MATRIX *src, MATRIX *dst);
void M3dMaths_RotMatrixYXZ(SVECTOR *angles, MATRIX *m);
void M3dMaths_ScaleMatrix(Item *pItem, MATRIX *m);

void M3d_BuildTransform(CSuper *t) {
	MATRIX *m = &t->mTransform;

	if (t->mExtraFlags & 1) {
		FUN_8008b68c(&((CBruce *)t)->mRotMatrix, m);

		m->m[0][1] = -(ushort)m->m[0][1];
		m->m[1][1] = -(ushort)m->m[1][1];
		m->m[2][1] = -(ushort)m->m[2][1];
	} else {
		M3dMaths_RotMatrixYXZ((SVECTOR *)&t->mAngles, m);
	}

	if (t->mFlags & 0x200)
		M3dMaths_ScaleMatrix((Item *)t, m);

	m->t[0] = t->mPos.vx >> 12;
	m->t[1] = t->mPos.vy >> 12;
	m->t[2] = t->mPos.vz >> 12;
}
