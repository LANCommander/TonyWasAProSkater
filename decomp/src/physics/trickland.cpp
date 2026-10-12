/* Trick_Land__6CBruce (0x8002f478) */
/* C++ (PHYSICS.CPP). Fuzzy demo twin Trick_Land (demo PHYSICS.CPP:605). The skater lands a trick:
   count the landing, score the spin as an "N OLLIE" (THPS1 names each spin amount where the demo
   computes -(n*50)), refresh the panel, fire the pending item, and feed the crowd meter into the
   special meter. */
#include "game/bruce.h"

extern int GGame;
extern char s_180_OLLIE[], s_360_OLLIE[], s_540_OLLIE[], s_720_OLLIE[], s_900_OLLIE[],
    s_1080_OLLIE[], s_MEGA_OLLIE[], s_Setting_Playtime_to_0[];

extern "C" {
	void FUN_8006e20c(CBruce *);
	int FUN_8004d83c(void *);
	int FUN_8004f000(Item *, CBruce *, int);
	void FUN_80010d84(int);
	void FUN_80042700(CBruce *, int, int, int, int);
	void FUN_8008c190(char *);
}

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
					Trick_Trick(74, s_180_OLLIE, -50, 0);
					break;
				case 2:
					Trick_Trick(74, s_360_OLLIE, -100, 0);
					break;
				case 3:
					Trick_Trick(74, s_540_OLLIE, -150, 0);
					break;
				case 4:
					Trick_Trick(74, s_720_OLLIE, -200, 0);
					break;
				case 5:
					Trick_Trick(74, s_900_OLLIE, -250, 0);
					break;
				case 6:
					Trick_Trick(74, s_1080_OLLIE, -300, 0);
					break;
				case 7:
					Trick_Trick(74, s_MEGA_OLLIE, -350, 0);
					break;
			}

			m364 = 1;
		}
	}

	FUN_8006e20c(this);

	if (m364) {
		if (mpWallItem && FUN_8004d83c(&mWallHandle)) {
			FUN_8004f000(mpWallItem, this, mPanel.mLandedTotal);

			mpWallItem = 0;
		}

		if (m364 && GGame == 7) {
			FUN_8008c190(s_Setting_Playtime_to_0);

			mTime = 0;
		}
	}

	FUN_80010d84(mPanel.mLandedTotal);
	unsigned Cheer = mPanel.mLandedTotal;

	if (m364) {
		if (!mBalanceFlash) {
			mBalanceTime += Cheer;

			if (mBalanceTime >= 3000) {
				mBalanceTime = 0;
				mBalanceFlash = 750;
				FUN_80042700(this, 20, 20, 20, 5);
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
