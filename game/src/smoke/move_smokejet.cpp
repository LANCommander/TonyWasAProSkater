/* Move__9CSmokeJet (0x80015c98) flags=-fstructure-noalias */
/* C++ (SMOKE.CPP). Fuzzy demo twin (0.97); members, locals and blocks from its MAIN.SYM. A smoke jet
   is a textured ribbon: switched off beyond 10000 units of the skater, otherwise each point
   wobbles sideways on its own sine, the widths ease toward randomly re-picked targets, and while
   it does damage a line test along the jet hits the skater (and optionally the baddies).
   Near-miss: retail loads the frame counter and sine-table base for mU, then copies them into the
   loop registers (t3/t4), as if the in-loop loads were hoisted and CSE'd. Ours never hoists a
   global load past a store in a loop, so the locals t/pTrig stand in (not in the demo SYM). */
#include "game/bit.h"
#include "game/ob.h"
#include "game/trig.h"

extern SSinCos rcossin_tbl[1];
extern int TTime;
extern CBody *MechList;
extern CBody *BaddyList;

int Redbook_Rnd(int);
void Utils_CalcUnit(CVector &, const CVector &);
CBody *M3dColij_LineToSphere(const CVector &, const CVector &, CVector &, CBody *, CBody *);

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
	mWidthWobble[0].TargetWidth = Redbook_Rnd(60) + 20;

	{
		int i;

		for (i = 1; i < 10; i++) {
			mpPoints[i].Width += (mWidthWobble[i].TargetWidth - mpPoints[i].Width) / 8;
			mpPoints[i].WidthB = mpPoints[i].Width + 40;

			if ((TTime & 15) == mWidthWobble[i].TChange) {
				mWidthWobble[i].TChange = Redbook_Rnd(16);
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
