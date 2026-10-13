/* Trick_Land__6CBruce (0x8002f478) */
/* C++ (PHYSICS.CPP). Fuzzy demo twin Trick_Land (demo PHYSICS.CPP:605). The skater lands a trick:
   count the landing, score the spin as an "N OLLIE" (THPS1 names each spin amount where the demo
   computes -(n*50)), refresh the panel, fire the pending item, and feed the crowd meter into the
   special meter. */
#include "game/bruce.h"
#include "game/crowd.h"
#include "game/mem.h"
#include "game/panel.h"
#include "game/psyq.h"
#include "game/trickob.h"

extern int GGame;

void CBruce::Trick_Land() {
	mTricks++;
	m720 = 0;

	if (!mNoSpinScore) {
		int Spin = mTallyAngles;

		if (((Spin ^ (Spin >> 31)) - (Spin >> 31)) > 1500)
			mNum180Spins++;
	}

	if (mNum180Spins == 1 && mOldPhysicsState == 2)
		mNum180Spins = 0;

	mPanel.TrickMessageRefresh();

	mNoSpinScore = 0;
	mNoSpin = 0;

	if (!m364 && mNum180Spins) {
		if (mOldPhysicsState != 2 || mNum180Spins >= 2) {
			int n = mNum180Spins;

			mNum180Spins = 0;

			switch (n) {
				case 1:
					Trick_Trick(74, "180 OLLIE", -50, 0);
					break;
				case 2:
					Trick_Trick(74, "360 OLLIE", -100, 0);
					break;
				case 3:
					Trick_Trick(74, "540 OLLIE", -150, 0);
					break;
				case 4:
					Trick_Trick(74, "720 OLLIE", -200, 0);
					break;
				case 5:
					Trick_Trick(74, "900 OLLIE", -250, 0);
					break;
				case 6:
					Trick_Trick(74, "1080 OLLIE", -300, 0);
					break;
				case 7:
					Trick_Trick(74, "MEGA OLLIE", -350, 0);
					break;
			}

			m364 = 1;
		}
	}

	Panel_Land(this);

	if (m364) {
		if (mpWallItem && Mem_CheckHandle(&mWallHandle)) {
			TrickOb_TrickedOnItem(mpWallItem, this, mPanel.mLandedTotal);

			mpWallItem = 0;
		}

		if (m364 && GGame == 7) {
			printf("Setting Playtime to 0\n");

			mTime = 0;
		}
	}

	Crowd_Cheer(mPanel.mLandedTotal);
	unsigned Cheer = mPanel.mLandedTotal;

	if (m364) {
		if (!mBalanceFlash) {
			mBalanceTime += Cheer;

			if (mBalanceTime >= 3000) {
				mBalanceTime = 0;
				mBalanceFlash = 750;
				SetGlow(20, 20, 20, 5);
			}
		} else {
			mBalanceFlash += Cheer / 5;

			if (mBalanceFlash > 750)
				mBalanceFlash = 750;
		}
	}

	m364 = 0;
	m368 = 0;
	mNum180Spins = 0;
	mTallyAngles = 0;
	m760 = 0;
}
