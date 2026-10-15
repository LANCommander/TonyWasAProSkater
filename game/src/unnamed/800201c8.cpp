/* FUN_800201c8 (0x800201c8) */
#include "game/powerup.h"

extern int TTime;

extern "C" void FUN_800201c8(CPowerUp *p) {
	p->mTimer++;

	if (p->mLifetime != 0xffff) {
		if (p->mLifetime != 0)
			p->mLifetime--;

		if (p->mLifetime < 60) {
			p->mWobbleAmplitude = (p->mMaxWobbleAmplitude * p->mLifetime) / 60;
		}

		if ((ushort)(p->mLifetime - 31) < 29) {
			if (TTime & 2)
				p->mFlags &= 0xfffe;
			else
				p->DontDisplay();
		}

		if (p->mLifetime < 31) {
			if (TTime & 1)
				p->mFlags &= 0xfffe;
			else
				p->DontDisplay();
		}

		if (p->mLifetime == 0)
			p->Die();
	}
}
