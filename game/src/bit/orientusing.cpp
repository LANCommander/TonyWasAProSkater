/* OrientUsing__8CQuadBitRC7CVectorRC7SVECTORiii (0x800284d8) */
#include "game/bit.h"

void CQuadBit::OrientUsing(const CVector &Position, const SVECTOR &Normal, int dx, int dy,
                           int Angle) {
	CVector b1;
	CVector b2;
	int l1;
	int l2;

	b1.vx = 0;
	b1.vy = -Normal.vz;
	b1.vz = Normal.vy;
	b2.vx = Normal.vy * Normal.vy + Normal.vz * Normal.vz;
	b2.vy = -Normal.vx * Normal.vy;
	b2.vz = -Normal.vz * Normal.vx;
	l1 = b1.Length();
	l2 = b2.Length();
	b1 = (b1 * dx) / l1;
	b2 = (b2 * dy) / l2;
	mPos = Position - b1 - b2;
	mPosB = Position + b1 - b2;
	mPosC = Position - b1 + b2;
	mPosD = Position + b1 + b2;
}
