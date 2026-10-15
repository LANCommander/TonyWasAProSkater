/* TrickTips_Init__Fv (0x8007236c) */
#include "game/panel.h"

extern CTrickTipMonitor TrickTipMonitor;

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
