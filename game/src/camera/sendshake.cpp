/* SendShake__FRC7CVectorP5CBody10EShakeType (0x800789a8) */
#include "game/camera.h"

void SendShake(const CVector &pos, CBody *b, EShakeType type) {
	while (b) {
		b->mInputFlags |= 2;
		b = (CBody *)b->mNextItem;
	}
}
