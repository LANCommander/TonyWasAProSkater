/* TRICKTIPS.CPP: the trick-tip monitor, which watches the skater's bails and tricks and shows a
   tip when one would help (declarations in game/tricktips.h). */

#include "game/tricktips.h"
#include "game/panel.h"
#include "game/mess.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/video.h"
#include "game/pad.h"
#include "game/globals.h"

extern CTrickTipMonitor TrickTipMonitor;
extern STip TRICKTIP[];
extern char *TEXT_TIP;
extern char *TEXT_REMEMBER;
extern char DAT_800a24a8[];
extern char DAT_800a2498[];
extern "C" void __dl__6CClassPv(void *p);

/* TrickTips_Display__Fv (0x80072164) */
void TrickTips_Display() {
	unsigned short y = 20;
	unsigned short t;

	if (TrickTipMonitor.mTipLine1) {
		t = 200 - TrickTipMonitor.mTipTimer;

		if (t < 10)
			y = y - (10 - t) * 40;

		if (TrickTipMonitor.mTipTimer < 10)
			y = y - (10 - TrickTipMonitor.mTipTimer) * 40;

		Mess_SetTextJustify(1);
		Mess_SetRGB(128, 128, 0, 0);
		Mess_SetScale(220);
		Mess_DrawPanelText(y, 100, TrickTipMonitor.mTipLine1, 0, 0, 256);

		if (TrickTipMonitor.mTipLine2)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 + 100, TrickTipMonitor.mTipLine2, 0, 0, 256);

		if (TrickTipMonitor.mTipLine3)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 2 + 100, TrickTipMonitor.mTipLine3, 0, 0, 256);

		if (TrickTipMonitor.mTipLine4)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 3 + 100, TrickTipMonitor.mTipLine4, 0, 0, 256);

		if (--TrickTipMonitor.mTipTimer == 0) {
			TrickTipMonitor.mTipLine4 = 0;
			TrickTipMonitor.mTipLine3 = 0;
			TrickTipMonitor.mTipLine2 = 0;
			TrickTipMonitor.mTipLine1 = 0;
		}
	}
}

/* TrickTips_Init__Fv (0x8007236c) */
void TrickTips_Init() {
	TrickTipMonitor.mTimer = 0;
	TrickTipMonitor.mLastTipTime = 0;
	TrickTipMonitor.mJumpTimer = 0;
	TrickTipMonitor.mBailCounter = 0;
	TrickTipMonitor.mPerpendicularBailCounter = 0;
	TrickTipMonitor.mTrickBailCounter = 0;
	TrickTipMonitor.mRailBailCounter = 0;
	TrickTipMonitor.mJumpCounter = 0;
	TrickTipMonitor.mWeakJumpCounter = 0;
	TrickTipMonitor.mAirCounter = 0;
	TrickTipMonitor.mTrickCounter = 0;
	TrickTipMonitor.mRailCounter = 0;
	TrickTipMonitor.mAirPolyCounter = 0;
	TrickTipMonitor.mAirPolyBreakCounter = 0;
	TrickTipMonitor.mGrindCounter = 0;
	TrickTipMonitor.mTipTimer = 0;
	TrickTipMonitor.mTipLine1 = 0;
	TrickTipMonitor.mTipLine2 = 0;
	TrickTipMonitor.mTipLine3 = 0;
	TrickTipMonitor.mTipLine4 = 0;
	TrickTipMonitor.mOldBails = 0;
	TrickTipMonitor.mOldPerpendicularBails = 0;
	TrickTipMonitor.mOldTrickBails = 0;
	TrickTipMonitor.mOldRailBails = 0;
	TrickTipMonitor.mOldJumps = 0;
	TrickTipMonitor.mOldWeakJumps = 0;
	TrickTipMonitor.mOldAirs = 0;
	TrickTipMonitor.mOldTricks = 0;
	TrickTipMonitor.mOldRails = 0;
	TrickTipMonitor.mOldAirPolys = 0;
	TrickTipMonitor.mOldAirPolyBreaks = 0;
	TrickTipMonitor.mOldGrinds = 0;
	TrickTipMonitor.mHoldingForwardCounter = 0;
	TrickTipMonitor.mOkayToShow = 1;
}

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

/* TrickTips_Update__Fv (0x80072404) */
/* Fuzzy demo twin (0.967); the SYM shows the body is all inlined helpers
   (tip, tip_mask, tip_used, tip_bank locals). The FindTip() path uses its own copy of the
   show-tip inline (ShowFoundTip): the SYM gives that copy tip_mask/tip_used/tip_bank declared at
   the top, and its sched1 order (TipsShown address first, tip_mask set twice so its load lands
   after the sllv) needs plain statements with the `tip_mask &= bit` split and no one-pass loop
   wrappers (their loop notes pinned the load above the sllv); the constant-tip copies keep the
   single-expression form. */
void TrickTips_Update() {
	TrickTipMonitor.Update();
}

/* FUN_80072e30 (0x80072e30) */
extern "C" void FUN_80072e30(void) {
	volatile int pad[4];

	*(char **)&TrickTipMonitor = DAT_800a24a8; /* the vtable pointer */
}

/* FUN_80072e4c (0x80072e4c) */
extern "C" void FUN_80072e4c(void) {
	*(char **)&TrickTipMonitor = DAT_800a2498; /* the vtable pointer */
}

/* FUN_80072e60 (0x80072e60) */
extern "C" void FUN_80072e60(char *p, int f) {
	*(char **)(p + 0) = DAT_800a24a8;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_80072e94 (0x80072e94) */
extern "C" void FUN_80072e94(char *p, int f) {
	*(char **)(p + 0) = DAT_800a24a8;

	if (f & 1)
		__dl__6CClassPv(p);
}
