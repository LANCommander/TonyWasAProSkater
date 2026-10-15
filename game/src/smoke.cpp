/* SMOKE.CPP: CSmokeJet, an object that emits smoke. Declarations in game/smoke.h. */
#include "game/smoke.h"
#include "game/bit.h"
#include "game/ob.h"
#include "game/trig.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/psyq.h"
#include "game/utils.h"
#include "game/bruce.h"

/* __9CSmokeJetRC7CVectorRC8CSVectoriUsiiiiUcUi (0x80015820) */
/* Classes and locals from the demo's MAIN.SYM; CBit's vptr sits at 0x44, after its data. */
CSmokeJet::CSmokeJet(const CVector &BasePos, const CSVector &Angles, int Length, ushort Duration,
                     int Damage, int r, int g, int b, uchar ScrollSpeed, uint Checksum)
    : CTexturedRibbon(10, 0) {
	mAngles = Angles;
	mLength = Length;
	SetPos(BasePos);

	mLifetime = Duration;
	mDamage = Damage;

	if (mDamage >= 1000) {
		mDamageBaddies = 1;
		mDamage = Damage - 1000;
	}

	mScrollSpeed = ScrollSpeed;
	SetTexture(Checksum);

	for (int i = 0; i < 10; i++) {
		int scaledr;
		int scaledg;
		int scaledb;

		mOrgX[i] = mpPoints[i].Pos.vx;
		mPhase[i] = Rnd(4096);
		mFreq[i] = 80;
		mAmp[i] = i ? Length / 80 : 0;
		mWidthWobble[i].TChange = Rnd(16);

		if (i == 0)
			mWidthWobble[0].TargetWidth = Rnd(60) + 20;
		else
			mWidthWobble[i].TargetWidth = mWidthWobble[i - 1].TargetWidth * 148 / 128;

		mpPoints[i].Width = mWidthWobble[i].TargetWidth;
		mpPoints[i].WidthB = mpPoints[i].Width + 40;
		scaledr = (9 - i) * r / 9;
		scaledg = (9 - i) * g / 9;
		scaledb = (9 - i) * b / 9;
		mpPoints[i].r = scaledr;
		mpPoints[i].g = scaledg;
		mpPoints[i].b = scaledb;
		scaledr = scaledr * 48 / 256;
		scaledg = scaledg * 48 / 256;
		scaledb = scaledb * 48 / 256;
		mpPoints[i].rB = scaledr;
		mpPoints[i].gB = scaledg;
		mpPoints[i].bB = scaledb;
	}
}

/* _._9CSmokeJet (0x80015b84) */
/* Nothing of its own to free; CTexturedRibbon's destructor does the rest. */
CSmokeJet::~CSmokeJet() {}

/* SetPos__9CSmokeJetRC7CVector (0x80015bd8) */
/* Lays the jet's ten ribbon points out in a straight line from the given position
   along its direction, mLength long. */
void CSmokeJet::SetPos(const CVector &pos) {
	CVector step, p;

	Utils_GetVecFromMagDir(step, mLength, mAngles);

	step /= 9;

	p = pos;

	for (int i = 0; i < 10; i++) {
		mpPoints[i].Pos = p;
		p += step;
	}
}

/* Move__9CSmokeJet (0x80015c98) */
/* Fuzzy demo twin (0.97); members, locals and blocks from its MAIN.SYM. A smoke jet
   is a textured ribbon: switched off beyond 10000 units of the skater, otherwise each point
   wobbles sideways on its own sine, the widths ease toward randomly re-picked targets, and while
   it does damage a line test along the jet hits the skater (and optionally the baddies).
   Near-miss: retail loads the frame counter and sine-table base for mU, then copies them into the
   loop registers (t3/t4), as if the in-loop loads were hoisted and CSE'd. Ours never hoists a
   global load past a store in a loop, so the locals t/pTrig stand in (not in the demo SYM). */
void CSmokeJet::Move() {
	if (mLifetime == 0xffff) {
		{
			int dx;
			int dz;
			int Dist;

			dx = (mpPoints[0].Pos.vx - MechList->mPos.vx) >> 12;

			if (dx < 0)
				dx = -dx;

			dz = (mpPoints[0].Pos.vz - MechList->mPos.vz) >> 12;

			if (dz < 0)
				dz = -dz;

			if (dx < dz)
				Dist = dz + dx / 2;
			else
				Dist = dx + dz / 2;

			if (Dist > 10000) {
				mDontDisplay = 1;
				return;
			}
		}
	} else {
		if (!mLifetime || !--mLifetime)
			Die();
	}

	mDontDisplay = 0;
	mU = rcossin_tbl[(TTime * 16) & 4080].s / 128;
	mV += mScrollSpeed;

	{
		int i;

		for (i = 0; i < 10; i++)
			mpPoints[i].Pos.vx =
			    mOrgX[i] +
			    (mAmp[i] * rcossin_tbl[(mFreq[i] * TTime + mPhase[i]) & 4095].s / 4096 << 12);
	}

	mpPoints[0].Width = 20;
	mpPoints[0].WidthB = 60;
	mWidthWobble[0].TargetWidth = Rnd(60) + 20;

	{
		int i;

		for (i = 1; i < 10; i++) {
			mpPoints[i].Width += (mWidthWobble[i].TargetWidth - mpPoints[i].Width) / 8;
			mpPoints[i].WidthB = mpPoints[i].Width + 40;

			if ((TTime & 15) == mWidthWobble[i].TChange) {
				mWidthWobble[i].TChange = Rnd(16);
				mWidthWobble[i].TargetWidth = mWidthWobble[i - 1].TargetWidth * 148 / 128;
			}
		}
	}

	if (!mDamageTimer || !--mDamageTimer) {
		if (mDamage) {
			CVector Start = mpPoints[0].Pos;
			CVector End = mpPoints[9].Pos;
			CVector Unit = End - Start;
			CVector dummy;
			CBody *pBody;

			Utils_CalcUnit(Unit, Unit);

			pBody = M3dColij_LineToSphere(Start, End, dummy, MechList, 0);

			if (pBody) {
				pBody->Hit(mDamage, Unit, 31);

				mDamageTimer = 15;
			}

			if (mDamageBaddies) {
				CBody *pBody = M3dColij_LineToSphere(Start, End, dummy, BaddyList, 0);

				if (pBody) {
					pBody->Hit(mDamage, Unit, 31);

					mDamageTimer = 15;
				}
			}
		}
	}
}
