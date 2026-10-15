/* TrickTips_Update__Fv (0x80072404) */
/* C++ (TRICKTIPS.CPP). Fuzzy demo twin (0.967); the SYM shows the body is all inlined helpers
   (tip, tip_mask, tip_used, tip_bank locals). The FindTip() path uses its own copy of the
   show-tip inline (ShowFoundTip): the SYM gives that copy tip_mask/tip_used/tip_bank declared at
   the top, and its sched1 order (TipsShown address first, tip_mask set twice so its load lands
   after the sllv) needs plain statements with the `tip_mask &= bit` split and no one-pass loop
   wrappers (their loop notes pinned the load above the sllv); the constant-tip copies keep the
   single-expression form. */
#include "game/panel.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/video.h"
#include "game/pad.h"

extern CTrickTipMonitor TrickTipMonitor;
extern SCareer GCareer;
extern SLevel Levels[];
extern STip TRICKTIP[];
extern SVideoInfo GVInfo1[];
extern SVideoInfo GVInfo1[2];
extern SControl Pad[2];
extern int DemoTimer;
extern int GGame;
extern CBruce *MechList;
extern int GLevel;

extern char *TEXT_TIP;
extern char *TEXT_REMEMBER;

static inline int TipBank(int tip) {
	int tip_bank;

	if (tip < 32)
		tip_bank = 0;
	else if (tip < 64)
		tip_bank = 1;

	return tip_bank;
}

inline int CTrickTipMonitor::FindTip() {
	int i;

	for (i = 0; i < 26; i++) {
		if (TRICKTIP[i].Flag)
			continue;

		if ((GCareer.Options.TrickTipUsed[TipBank(i)] >> (i - TipBank(i) * 32)) & 1)
			continue;

		return i;
	}

	return -1;
}

inline void CTrickTipMonitor::ShowTip(int tip) {
	if (!mTimer || mTimer >= mLastTipTime + 600) {
		mLastTipTime = mTimer;
		int tip_bank = TipBank(tip);

		if (GCareer.Options.TrickTipsEnabled) {
			if (((Levels[GLevel].TrickTipMask[tip_bank] >> (tip - tip_bank * 32)) & 1) == 0) {
				int &seen = GCareer.Options.TrickTipUsed[tip_bank];
				int tip_mask =
				    GCareer.Options.TrickTipMask[tip_bank] & (1 << (tip - tip_bank * 32));
				int tip_used = seen & (1 << (tip - tip_bank * 32));

				seen |= (1 << (tip - tip_bank * 32));

				if (!tip_mask) {
					if (tip_used) {
						mTipLine1 = TEXT_REMEMBER;
						GCareer.Options.TrickTipMask[tip_bank] |= 1 << (tip - tip_bank * 32);
					} else {
						mTipLine1 = TEXT_TIP;
					}

					mTipTimer = 200;
					mTipLine2 = TRICKTIP[tip].Line1;
					mTipLine3 = TRICKTIP[tip].Line2;
					mTipLine4 = TRICKTIP[tip].Line3;
				}
			}
		}
	}
}

inline void CTrickTipMonitor::ShowFoundTip(int tip) {
	int tip_mask;
	int tip_used;
	int tip_bank;

	if (!mTimer || mTimer >= mLastTipTime + 600) {
		mLastTipTime = mTimer;
		tip_bank = TipBank(tip);

		if (GCareer.Options.TrickTipsEnabled) {
			if (((Levels[GLevel].TrickTipMask[tip_bank] >> (tip - tip_bank * 32)) & 1) == 0) {
				tip_mask = GCareer.Options.TrickTipMask[tip_bank];
				tip_used = GCareer.Options.TrickTipUsed[tip_bank];
				tip_mask &= (1 << (tip - tip_bank * 32));
				GCareer.Options.TrickTipUsed[tip_bank] |= (1 << (tip - tip_bank * 32));
				tip_used &= (1 << (tip - tip_bank * 32));

				if (!tip_mask) {
					if (tip_used) {
						mTipLine1 = TEXT_REMEMBER;
						GCareer.Options.TrickTipMask[tip_bank] |= 1 << (tip - tip_bank * 32);
					} else {
						mTipLine1 = TEXT_TIP;
					}

					mTipTimer = 200;
					mTipLine2 = TRICKTIP[tip].Line1;
					mTipLine3 = TRICKTIP[tip].Line2;
					mTipLine4 = TRICKTIP[tip].Line3;
				}
			}
		}
	}
}

inline void CTrickTipMonitor::Update() {
	if (Levels[GLevel].isCompetition)
		return;

	if (GVInfo1[0].VideoMode == 2 || GVInfo1[1].VideoMode == 2)
		return;

	if (!DemoTimer || GGame != 1)
		return;

	pSkater = MechList;

	if (!mTimer) {
		int tip = FindTip();

		if (tip >= 0)
			ShowFoundTip(tip);
	}

	mTimer++;
	mJumpTimer++;

	if (mOldTricks < pSkater->mTricks)
		mOldTricks = pSkater->mTricks;

	if (mOldGrinds < pSkater->mGrinds) {
		mOldGrinds = pSkater->mGrinds;
		mGrindCounter++;
		GCareer.Options.TrickTipUsed[0] |= 0x100;
		GCareer.Options.TrickTipMask[0] |= 0x100;
	}

	if (mOldJumps < pSkater->mJumps) {
		mOldJumps = pSkater->mJumps;
		mJumpTimer = 0;
		mJumpCounter++;
	}

	if (mJumpTimer > 600)
		mJumpTimer = 0;

	if (Pad[0].Up.Pressed || Pad[1].Up.Pressed)
		mHoldingForwardCounter++;
	else
		mHoldingForwardCounter = 0;

	if (mHoldingForwardCounter > 150) {
		ShowTip(1);

		mHoldingForwardCounter = 0;
	}

	if (mOldAirPolyBreaks < pSkater->mAirPolyBreaks) {
		mOldAirPolyBreaks = pSkater->mAirPolyBreaks;

		if (++mAirPolyBreakCounter >= 3) {
			mAirPolyBreakCounter = 0;

			if (mBigAirCounter < 3)
				ShowTip(6);
		}
	}

	if (mOldAirPolys < pSkater->mAirPolys) {
		mOldAirPolys = pSkater->mAirPolys;
		mAirPolyCounter++;
	}

	if (mOldBigAirs < pSkater->mBigAirs) {
		mOldBigAirs = pSkater->mBigAirs;
		mBigAirCounter++;
	}

	if (mOldWeakJumps < pSkater->mWeakJumps) {
		mOldWeakJumps = pSkater->mWeakJumps;

		if (++mWeakJumpCounter >= 5) {
			ShowTip(5);

			mWeakJumpCounter = 0;
		}
	}

	if (mOldBails < pSkater->mBails) {
		mOldBails = pSkater->mBails;

		if (++mBailCounter >= 3) {
			ShowTip(3);

			mBailCounter = 0;
		}
	}

	if (mOldPerpendicularBails < pSkater->mPerpendicularBails) {
		mOldPerpendicularBails = pSkater->mPerpendicularBails;

		if (++mPerpendicularBailCounter >= 3) {
			ShowTip(2);

			mPerpendicularBailCounter = 0;
		}
	}

	if (mOldRailBails < pSkater->mRailBails) {
		mOldRailBails = pSkater->mRailBails;

		if (++mRailBailCounter >= 3) {
			ShowTip(9);

			mRailBailCounter = 0;
		}
	}
}

void TrickTips_Update() {
	TrickTipMonitor.Update();
}
