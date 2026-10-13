/* Trick_TakeOff__6CBruce (0x8002f1a0) */
/* C++ (PHYSICS.CPP). Leaving the ground: unless wall-riding or out of time, starts the air
   (m760, m1FC = 15), resets the trick buffers (not when coming off an invisible surface), notes
   whether the wall item is a trick object (m34C), clears the spin score, resets the panel's
   combo (Panel_TakeOff) and the per-combo trick counts (and in game mode 2 the per-run ones). */
#include "game/bruce.h"
#include "game/panel.h"
#include "game/trickob.h"

extern int GGame;

void CBruce::Trick_TakeOff() {
	int i;

	if (mPhysicsState == PHYSICS_IN_WALLRIDE)
		return;

	if (!mTime)
		return;

	m760 = 1;
	m200 = 0;
	m1FC = 15;

	if (mPhysicsState != PHYSICS_ON_INVISIBLE)
		ResetTrickBuffers();

	m34C = 0;

	if (mpWallItem) {
		if (Mem_CheckHandle(&mWallHandle))
			m34C = TrickOb_GetItemType(mpWallItem);
	}

	mNoSpinScore = 0;
	mNoSpin = 0;
	Panel_TakeOff(this, 0);

	for (i = 0; i < 80; i++)
		m568[i] = 0;

	if (GGame == 2) {
		int j;

		for (j = 0; j < 80; j++)
			m428[j] = 0;
	}
}
