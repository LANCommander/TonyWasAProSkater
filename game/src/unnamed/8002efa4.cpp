/* FUN_8002efa4 (0x8002efa4) */
/* C++. Before a skater is drawn: keeps its rotation and position (mTempRotMatrix, mTempPos) and,
   while it flips (m720), turns it by the flip angle about a point 0x46000 up its own y axis,
   moving it so that point stays put; then builds its transform. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/bruce.h"

void M3d_BuildTransform(CSuper *);

extern "C" void FUN_8002efa4(CBruce *p) {
	MATRIX m;
	SVECTOR ang;
	VECTOR a;
	VECTOR b;
	VECTOR c1;
	VECTOR c2;

	p->mTempRotMatrix = p->mRotMatrix;
	p->mTempPos = p->mPos;

	if (p->m720) {
		a.vz = 0;
		a.vy = 0;
		a.vx = 0;
		TransMatrix(&p->mRotMatrix, &a);
		TransMatrix(&m, &a);

		b.vx = 0;
		b.vy = 0x46000;
		b.vz = 0;
		ApplyMatrixLV(&p->mRotMatrix, &b, &c1);

		ang.vz = 0;
		ang.vy = 0;
		ang.vx = (ushort)p->m724 & 4095;
		RotMatrixYXZ(&ang, &m);
		MulMatrix(&p->mRotMatrix, &m);
		ApplyMatrixLV(&p->mRotMatrix, &b, &c2);

		p->mPos.vx = p->mPos.vx + c1.vx - c2.vx;
		p->mPos.vy = p->mPos.vy + c1.vy - c2.vy;
		p->mPos.vz = p->mPos.vz + c1.vz - c2.vz;
	}

	M3d_BuildTransform(p);
}
