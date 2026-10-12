/* HandleTricksNew__6CBruce (0x80031758) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Called from DoPhysics. THPS1's whole trick-input handler; the THPS2 demo's
   HandleTricksNew is a 76-byte wrapper around its new button-queue code, so there is no usable
   twin. Records the d-pad direction and the Circle/Square buttons into 10-slot ring buffers
   (m378 dirs, m3A0 buttons, m3C8/m3F0 their times), with m25C..m280 remembering the previous
   direction for special (two-direction) combos. A buffered entry starts a trick: specials are
   searched from trick 32 by style/combo/dir/button key (needs special meter mBalanceFlash), otherwise
   Circle = flip T[dir+16], Square = grab T[dir]. Then the held-trick state machine: the 0x40000000
   (held anim) / 0x20000000 / 0x80000000 states release into anim 14. Line numbers are the
   Debug_RunAnim __LINE__s.
   Levers: the paired ring-slot clears are chained (`m378[k] = m3A0[k] = 0;`) so the index is
   loaded once; the special-trick search walks a pointer (`p = &T[32], i = 32; ...; i++, p++`);
   mAnimSpeed is read before m368 is set; the combo-timing test computes Gap in a nested block. */
#include "game/bruce.h"
#include "game/career.h"

extern SStats Stats[];
extern STrick Group[];
extern int XFrames;

extern "C" {
	void FUN_8003159c(CBruce *);
	void FUN_80031500(CBruce *);
}

void CBruce::HandleTricksNew() {
	if (!mDoingATrick && !m740)
		m424 = 0;

	if (m200) {
		m200--;
		return;
	}

	if (m724)
		return;

	int Dir = mpPad->Up.Pressed + mpPad->Down.Pressed * 2 + mpPad->Left.Pressed * 4 +
	          mpPad->Right.Pressed * 8;

	if (mAY < -40)
		Dir |= 1;

	if (mAY > 40)
		Dir |= 2;

	if (mAX < -40)
		Dir |= 4;

	if (mAX > 40)
		Dir |= 8;

	if (Dir) {
		if (m280) {
			if (m27C) {
				m25C = m268;
				m260 = m26C;
				m264 = m270;
				m268 = m278;
				m26C = m274;
				m270 = m27C;
				m274 = 0;
				m27C = 0;
			}

			if (!m274) {
				m278 = Dir;
				m27C = 0;
				m274 = XFrames;
			}
		}

		m378[m370] = Dir;
		m3C8[m370] = XFrames;

		if (m3A0[m370] && XFrames - m3F0[m370] > 20)
			m3A0[m370] = 0;
	} else if (m274 && !m27C) {
		m27C = XFrames;
	} else {
		m280 = 1;
	}

	if (mInAir || !m740 || mPhysicsState == 4 || mPhysicsState == 5 || mPhysicsState == 8) {
		m374 = 0;
		return;
	}

	int Button = mpPad->Circle.Pressed + mpPad->Square.Pressed * 2;

	if (Button) {
		m3A0[m370] = Button;
		m3F0[m370] = XFrames;

		if (m378[m370] && XFrames - m3C8[m370] > 20)
			m378[m370] = 0;
	}

	if (!Button) {
		if (m374 > 0) {
			m3A0[m370] = m378[m370] = 0;
			m374--;
		}
	}

	if (m378[m370] && m3A0[m370] && !m374) {
		if (m270 && m270 < m3C8[m370]) {
			int Gap = m3C8[m370] - m270;

			if (m270 - m26C < 20 && Gap < 30)
				m378[m370] |= m268 << 4;
		}

		m264 = 0;
		m260 = 0;
		m280 = 0;
		m27C = 0;
		m270 = 0;
		m274 = 0;
		m26C = 0;

		if (++m370 == 10)
			m370 = 0;

		m378[m370] = m3A0[m370] = 0;
		m374 = 3;
	}

	if (m36C != m370 && (!mDoingATrick || m360)) {
		int Entry = m378[m36C];

		Button = m3A0[m36C];
		Dir = Entry & 15;
		int Combo = Entry >> 4;

		if (!mBalanceFlash)
			Combo = 0;

		if (Combo) {
			int Key = Stats[mSkaterIndex].ComboID + (Combo << 8) + (Dir << 4) + Button;
			int i;
			STrick *p;

			for (p = &Group[32], i = 32; p->Flags > 0; i++, p++) {
				if ((p->Flags & 0xfffff) == Key) {
					mAnimSpeed = p->w2;

					if (Button == 2)
						m368 = i | 0x2000;
					else
						m368 = i | 0x4000;

					if (p->Frames) {
						Debug_RunAnim2(p->Anim, 0, p->Frames, 2335);

						m368 |= 0x40000000;
					} else {
						Debug_RunAnim(p->Anim, 2341);

						if (Button == 2)
							m368 |= 0x80000000;
						else
							m368 |= 0x20000000;
					}

					SetTrickFlags(p);

					m720 = p->w8;
					m728 = p->w9;

					if (m728 < 0)
						m720++;

					m724 = 0;

					if (mExtraFlags & 2)
						mTapTurn -= p->w10;
					else
						mTapTurn += p->w10;

					mDoingATrick = 1;
					m364 = 0;
					m360 = 0;
					m348 = XFrames;
					m378[m36C] = m3A0[m36C] = 0;

					if (++m36C == 10)
						m36C = 0;

					break;
				}
			}

			if (p->Flags == -1)
				Combo = 0;
		}

		if (!Combo && (Button & 1) && Group[Dir].Anim != 78) {
			Dir += 16;
			mAnimSpeed = Group[Dir].w2;
			m368 = Dir | 0x4000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2(Group[Dir].Anim, 0, Group[Dir].Frames, 2396);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim(Group[Dir].Anim, 2401);

				m368 |= 0x20000000;
			}

			mDoingATrick = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		} else if (!Combo && (Button & 2) && Group[Dir].Anim != 78) {
			mAnimSpeed = Group[Dir].w2;
			m368 = Dir | 0x2000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2(Group[Dir].Anim, 0, Group[Dir].Frames, 2423);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim(Group[Dir].Anim, 2428);

				m368 |= 0x80000000;
			}

			mDoingATrick = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		}
	}

	if (!mDoingATrick)
		return;

	int State = m368 & 0xff000000;
	int Trick = m368 & 0xff;

	if (State == 0x20000000 && mAnimFinished)
		m360 = 1;

	if (State == 0x40000000 && mAnimFinished)
		m360 = 1;

	if (mAnimDir == -1)
		m360 = 1;
	else {
		if ((m368 & 0x4000) && Group[Trick].Frames &&
		    mFrame >= mNumFrames - Group[Trick].w5 - 1)
			m360 = 1;

		if ((m368 & 0x2000) && mFrame >= mNumFrames - Group[Trick].w5 - 1)
			m360 = 1;
	}

	if (m360) {
		FUN_8003159c(this);

		m364 = 1;
	}

	if (!m1EC)
		FUN_80031500(this);

	if (!mAnimFinished)
		return;

	State = m368 & 0xff000000;
	Trick = m368 & 0xff;

	switch (State) {
		case 0x40000000:
			if (m368 & 0x4000) {
				if (mpPad->Circle.Pressed)
					m368 |= 0x800;

				if (!mpPad->Circle.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2(mAnim, Group[Trick].Frames, -1, 2529);
					else if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2535);

						mDoingATrick = 0;
					}
				}
			} else {
				if (mpPad->Square.Pressed)
					m368 |= 0x800;

				if (!mpPad->Square.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2(mAnim, Group[Trick].Frames, -1, 2553);
					else if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2559);

						mDoingATrick = 0;
					}
				}
			}

			break;
		case 0x20000000:
			if (mpPad->Circle.Pressed)
				m368 |= 0x800;

			if (!mpPad->Circle.Pressed)
				m368 &= ~0x800;

			if (!(m368 & 0x800)) {
				if (m368 & 0x400) {
					if (!m6DC) {
						mAnimSpeed = 0x10000;
						Debug_RunAnim(14, 2580);

						mDoingATrick = 0;
					}
				} else {
					Debug_RunAnim2(mAnim, mFrame, 0, 2586);

					m368 |= 0x400;
				}
			}

			break;
		case (int)0x80000000:
			if (!m6DC) {
				mAnimSpeed = 0x10000;
				Debug_RunAnim(14, 2596);

				mDoingATrick = 0;
			}

			break;
	}
}
