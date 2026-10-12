/* AI__16CScriptOnlyBaddy (0x80041388) */
/* C++ (BADDY.CPP). Fuzzy demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets. */
#include "game/baddy.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/m3d.h"

extern int SFXLEVEL;
extern volatile int TTime;
extern CBruce *MechList;
extern CBody *CameraList;

void SFX_ModifyPos(unsigned int, const CVector &, int);
int Rnd(int);

void CScriptOnlyBaddy::AI() {
	mFlags |= 0x41;
	mInputFlagsCopy = mInputFlags;

	if (mInputFlags & 1) {
		mInputFlags &= 0xfffe;
		mInputSignal++;
	}

	if (mDoScript) {
		if (mWaitTimer)
			mWaitTimer--;
		else
			ParseScript(mpScript);
	} else {
		Die();
	}

	if (mLoopingSoundUpdate != -1 && TTime % mLoopingSoundUpdate == 0) {
		int t = SFXLEVEL;
		unsigned int snd = mLoopingSound;

		SFXLEVEL = t * mLoopingSoundVol / 100;
		SFX_ModifyPos(snd, mPos, 0);

		SFXLEVEL = t;
	}

	if (mpWeather) {
		int ground = MechList->mShadowPos.vy;

		for (int i = 0; i < mWeatherData; i++) {
			SRainEntry *p_entry = &mpWeather[i];
			CVector pos = p_entry->p_line->mStart;

			pos += p_entry->vel;

			if (ground < pos.vy) {
				CVector offset((Rnd(2400) - 1200) << 12, (Rnd(400) - 1200) << 12,
				               (Rnd(2400) - 1200) << 12);

				pos = MechList->mPos + offset;
			}

			p_entry->p_line->mStart = pos;
			p_entry->p_line->mSegs->End = (pos - p_entry->vel) + (CameraList->mVel >> 1);
		}
	}
}
