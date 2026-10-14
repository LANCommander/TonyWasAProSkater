/* CalculateWorldCoords__9CChunkBit (0x80028b6c) */
/* C++ (BIT.CPP). Rotates the chunk's four corners by its angles and translates them to its position,
   giving the four world-space corners. */
#include <sys/types.h>
#include <libgte.h>
#ifndef PC_PORT
#include <inline_c.h>
#endif
#include "game/bit.h"
#include "game/m3dasm.h"

void CChunkBit::CalculateWorldCoords() {
	MATRIX m;

	M3dMaths_RotMatrixYXZ((SVECTOR *)&mAngles, &m);
	SetRotMatrix(&m);
	M3dAsm_SetTransVector((Vector *)&mPos);
#ifdef PC_PORT
	geom_transform_sv(&mPosA, &mWorldPosA);
	geom_transform_sv(&mPosB, &mWorldPosB);
	geom_transform_sv(&mPosC, &mWorldPosC);
	geom_transform_sv(&mPosD, &mWorldPosD);
#else
	gte_ldv0(&mPosA);
	gte_rtv0tr();
	gte_stlvnl(&mWorldPosA);
	gte_ldv0(&mPosB);
	gte_rtv0tr();
	gte_stlvnl(&mWorldPosB);
	gte_ldv0(&mPosC);
	gte_rtv0tr();
	gte_stlvnl(&mWorldPosC);
	gte_ldv0(&mPosD);
	gte_rtv0tr();
	gte_stlvnl(&mWorldPosD);
#endif
}
