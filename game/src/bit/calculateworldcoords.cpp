/* CalculateWorldCoords__9CChunkBit (0x80028b6c) */
/* C++ (BIT.CPP). Rotates the chunk's four corners by its angles and translates them to its position,
   giving the four world-space corners. */
#include <sys/types.h>
#include <libgte.h>
#include "game/bit.h"
#include "game/m3dasm.h"

void CChunkBit::CalculateWorldCoords() {
	MATRIX m;

	M3dMaths_RotMatrixYXZ((SVECTOR *)&mAngles, &m);
	SetRotMatrix(&m);
	M3dAsm_SetTransVector((Vector *)&mPos);
	geom_transform_sv(&mPosA, &mWorldPosA);
	geom_transform_sv(&mPosB, &mWorldPosB);
	geom_transform_sv(&mPosC, &mWorldPosC);
	geom_transform_sv(&mPosD, &mWorldPosD);
}
