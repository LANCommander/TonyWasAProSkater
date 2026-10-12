/* SetPos__9CSmokeJetRC7CVector (0x80015bd8) */
/* C++ (SMOKE.CPP). Lays the jet's ten ribbon points out in a straight line from the given position
   along its direction, mLength long. */
#include "game/bit.h"

void Utils_GetVecFromMagDir(CVector &, int, const CSVector &);

void CSmokeJet::SetPos(const CVector &pos) {
	CVector step, p;

	Utils_GetVecFromMagDir(step, mLength, mAngles);
	step /= 9;

	p = pos;

	for (int i = 0; i < 10; i++) {
		mpPoints[i].Pos = p;
		p += step;
	}
}
