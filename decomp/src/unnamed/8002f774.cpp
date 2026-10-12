/* Trick_Bail__6CBruce (0x8002f774) */
/* C++ (PHYSICS.CPP). A bail ends the combo: clears trick state, cheers or groans the crowd by the
   trick queue's score (or the bail speed), forgets this combo's trick counts, and rumbles. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/bruce.h"

struct Message;

extern int GGame;

int Mips_CVecDotProd(CVector *, CVector *);

extern "C" {
	void FUN_80042700(CBruce *, int, int, int, int);
	int FUN_8008b5dc(int);
	void FUN_80010f70(int);
	void FUN_80010eb8(int);
	void FUN_8006e22c(CBruce *);
}

void CBruce::Trick_Bail() {
	int i, Score, Speed, Spins;

	if (GGame == 7)
		mTime = 0;

	m760 = 0;
	mNum180Spins = 0;
	mTallyAngles = 0;
	mGaps = 0;
	mpGrindGapTrick = 0;
	m720 = 0;
	mNoSpinScore = 0;
	mNoSpin = 0;
	mBalanceTime = 0;
	mBalanceFlash = 0;
	FUN_80042700(this, 0, 0, 0, 10);

	Speed = FUN_8008b5dc(Mips_CVecDotProd(&mVel, &mVel)) << 6;
	Score = 0;

	for (i = 0; i < mPanel.mTQStack; i++)
		Score += mPanel.mTQ[i].point_value;

	Spins = mNum180Spins;

	if (Spins < 3)
		Score = Score * (mPanel.mTQStack * 2 + Spins) / 2;
	else
		Score = (Score * (mPanel.mTQStack * 2 + (Spins * 2 - 2))) >> 1;

	if (Score != 0)
		FUN_80010f70(Score);
	else
		FUN_80010eb8(Speed >> 12);

	for (i = 0; i < 80; i++) {
		m428[i] -= m568[i];
		m568[i] = 0;
	}

	FUN_8006e22c(this);

	m364 = 0;
	m368 = 0;
	Vibrate(8, 1, 150);
}
