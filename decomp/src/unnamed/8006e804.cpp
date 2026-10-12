/* Trick__10CPanelDataPcii (0x8006e804) flags=-fstructure-noalias */
/* C++ (PANEL.CPP). Demo twin CPanelData::Trick (656 B); THPS1 drops the spin/lock bookkeeping,
   shrinks CTrick to 20 bytes and adds the trick-status check. Queues a trick on the panel's trick
   queue, banking the previous landed total when the queue is empty. FUN_8006e514 = CTrick::Set,
   FUN_8006e7c0 = TrickMessage, the panel refresh. Called via the CBruce wrapper at 0x8006e19c (&mPanel = +0x924). */
#include "game/panel.h"

int DAT_800d1ec4; /* points x10 cheat */

extern char s_Unexpected_trick_status_d[]; /* "Unexpected trick status: %d\n" */

extern "C" int FUN_8008c190(const char *, ...);

void CPanelData::Trick(char *pName, int Points, int Dep) {
	mLastHoldingScore = 1;

	if (DAT_800d1ec4)
		Points *= 10;

	switch (mTrickStatus) {
		case 0:
		case 1:
		case 2:
			if (Points > 0) {
				if (!mTQStack) {
					mTotalScore += mLandedTotal;
					mLandedTotal = 0;
					mLandedMultiplier = 0;
					m4F4 = 0;
				}

				mTQ[mTQStack].Set(0, pName, Points, Dep);

				if (mTQStack < 19)
					mTQStack++;
			}

			break;
		default:
			FUN_8008c190(s_Unexpected_trick_status_d, mTrickStatus);
			break;
	}

	TrickMessage();
}
