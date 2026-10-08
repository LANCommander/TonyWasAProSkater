/* HandleTricksNew__6CBruce (0x80031758) flags=-fstructure-noalias */
/* C++ (PHYSICS.CPP). Called from DoPhysics. THPS1's whole trick-input handler; the THPS2 demo's
   HandleTricksNew is a 76-byte wrapper around its new button-queue code, so there is no usable
   twin. Records the d-pad direction and the Circle/Square buttons into 10-slot ring buffers
   (m378 dirs, m3A0 buttons, m3C8/m3F0 their times), with m25C..m280 remembering the previous
   direction for special (two-direction) combos. A buffered entry starts a trick: specials are
   searched from trick 32 by style/combo/dir/button key (needs special meter m254), otherwise
   Circle = flip T[dir+16], Square = grab T[dir]. Then the held-trick state machine: the 0x40000000
   (held anim) / 0x20000000 / 0x80000000 states release into anim 14. Line numbers are the
   Debug_RunAnim __LINE__s.
   Levers: the paired ring-slot clears are chained (`m378[k] = m3A0[k] = 0;`) so the index is
   loaded once; the special-trick search walks a pointer (`p = &T[32], i = 32; ...; i++, p++`);
   m150 is read before m368 is set; the combo-timing test computes Gap in a nested block. */
typedef unsigned short ushort;
typedef unsigned char uchar;

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle; /* 0x00 */
	SButton Square;   /* 0x10 */
	SButton Circle;   /* 0x20 */
	SButton X;        /* 0x30 */
	SButton LeftOne;  /* 0x40 */
	SButton LeftTwo;  /* 0x50 */
	SButton RightOne; /* 0x60 */
	SButton RightTwo; /* 0x70 */
	SButton Left;     /* 0x80 */
	SButton Right;    /* 0x90 */
	SButton Up;       /* 0xa0 */
	SButton Down;     /* 0xb0 */
};

struct SStats {
	char pad0[0xc];
	int Style; /* 0x0c */
	char pad10[340 - 0x10];
};

struct STrick {
	int Flags;    /* 0x00 */
	int Anim;     /* 0x04 */
	int w2;       /* 0x08 */
	char *pName;  /* 0x0c */
	int Frames;   /* 0x10 */
	int w5;       /* 0x14 */
	int MinFrame; /* 0x18 */
	int Score;    /* 0x1c */
	int w8;       /* 0x20 */
	int w9;       /* 0x24 */
	int w10;      /* 0x28 */
};

extern SStats Stats[];
extern STrick Group[];
extern int XFrames;

class CBruce {
  public:
	char pad0[0x134];
	int mFlags; /* 0x134 */
	char pad138[0x144 - 0x138];
	short mFrame; /* 0x144 */
	ushort mAnim; /* 0x146 */
	char pad148;
	signed char m149; /* 0x149 */
	char pad14a[0x14e - 0x14a];
	uchar m14E; /* 0x14e */
	uchar m14F; /* 0x14f */
	int m150;   /* 0x150 */
	char pad154[0x1ec - 0x154];
	int m1EC; /* 0x1ec */
	char pad1f0[0x200 - 0x1f0];
	int m200; /* 0x200 */
	char pad204[0x254 - 0x204];
	int m254; /* 0x254 */
	char pad258[0x25c - 0x258];
	int m25C; /* 0x25c */
	int m260; /* 0x260 */
	int m264; /* 0x264 */
	int m268; /* 0x268 */
	int m26C; /* 0x26c */
	int m270; /* 0x270 */
	int m274; /* 0x274 */
	int m278; /* 0x278 */
	int m27C; /* 0x27c */
	int m280; /* 0x280 */
	char pad284[0x2e8 - 0x284];
	int mSkaterIndex; /* 0x2e8 */
	char pad2ec[0x324 - 0x2ec];
	int m324; /* 0x324 */
	char pad328[0x348 - 0x328];
	int m348; /* 0x348 */
	char pad34c[0x358 - 0x34c];
	int m358;     /* 0x358 */
	int m35C;     /* 0x35c */
	int m360;     /* 0x360 */
	int m364;     /* 0x364 */
	int m368;     /* 0x368 */
	int m36C;     /* 0x36c */
	int m370;     /* 0x370 */
	int m374;     /* 0x374 */
	int m378[10]; /* 0x378 */
	int m3A0[10]; /* 0x3a0 */
	int m3C8[10]; /* 0x3c8 */
	int m3F0[10]; /* 0x3f0 */
	char pad418[0x424 - 0x418];
	int m424; /* 0x424 */
	char pad428[0x6dc - 0x428];
	int m6DC; /* 0x6dc */
	char pad6e0[0x720 - 0x6e0];
	int m720; /* 0x720 */
	int m724; /* 0x724 */
	int m728; /* 0x728 */
	char pad72c[0x734 - 0x72c];
	int mPhysicsState; /* 0x734 */
	char pad738[0x740 - 0x738];
	int m740; /* 0x740 */
	char pad744[0x86c - 0x744];
	SControl *mPad; /* 0x86c */
	char pad870[0x894 - 0x870];
	signed char mAX; /* 0x894 */
	signed char mAY; /* 0x895 */
	void SetTrickFlags(STrick *);
	void HandleTricksNew();
};

extern "C" {
	void Debug_RunAnim__6CBruceii(CBruce *, int, int);
	void Debug_RunAnim2__6CBruceiiii(CBruce *, int, int, int, int);
	void FUN_8003159c(CBruce *);
	void FUN_80031500(CBruce *);
}

void CBruce::HandleTricksNew() {
	if (!m35C && !m740)
		m424 = 0;

	if (m200) {
		m200--;
		return;
	}

	if (m724)
		return;

	int Dir = mPad->Up.Pressed + mPad->Down.Pressed * 2 + mPad->Left.Pressed * 4 +
	          mPad->Right.Pressed * 8;

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

	if (m358 || !m740 || mPhysicsState == 4 || mPhysicsState == 5 || mPhysicsState == 8) {
		m374 = 0;
		return;
	}

	int Button = mPad->Circle.Pressed + mPad->Square.Pressed * 2;

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

	if (m36C != m370 && (!m35C || m360)) {
		int Entry = m378[m36C];

		Button = m3A0[m36C];
		Dir = Entry & 15;
		int Combo = Entry >> 4;

		if (!m254)
			Combo = 0;

		if (Combo) {
			int Key = Stats[mSkaterIndex].Style + (Combo << 8) + (Dir << 4) + Button;
			int i;
			STrick *p;

			for (p = &Group[32], i = 32; p->Flags > 0; i++, p++) {
				if ((p->Flags & 0xfffff) == Key) {
					m150 = p->w2;

					if (Button == 2)
						m368 = i | 0x2000;
					else
						m368 = i | 0x4000;

					if (p->Frames) {
						Debug_RunAnim2__6CBruceiiii(this, p->Anim, 0, p->Frames, 2335);

						m368 |= 0x40000000;
					} else {
						Debug_RunAnim__6CBruceii(this, p->Anim, 2341);

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

					if (mFlags & 2)
						m324 -= p->w10;
					else
						m324 += p->w10;

					m35C = 1;
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
			m150 = Group[Dir].w2;
			m368 = Dir | 0x4000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2__6CBruceiiii(this, Group[Dir].Anim, 0, Group[Dir].Frames, 2396);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim__6CBruceii(this, Group[Dir].Anim, 2401);

				m368 |= 0x20000000;
			}

			m35C = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		} else if (!Combo && (Button & 2) && Group[Dir].Anim != 78) {
			m150 = Group[Dir].w2;
			m368 = Dir | 0x2000;

			if (Group[Dir].Frames) {
				Debug_RunAnim2__6CBruceiiii(this, Group[Dir].Anim, 0, Group[Dir].Frames, 2423);

				m368 |= 0x40000000;
			} else {
				Debug_RunAnim__6CBruceii(this, Group[Dir].Anim, 2428);

				m368 |= 0x80000000;
			}

			m35C = 1;
			m364 = 0;
			m360 = 0;
			m348 = XFrames;
			m378[m36C] = m3A0[m36C] = 0;

			if (++m36C == 10)
				m36C = 0;

			SetTrickFlags(&Group[Dir]);
		}
	}

	if (!m35C)
		return;

	int State = m368 & 0xff000000;
	int Trick = m368 & 0xff;

	if (State == 0x20000000 && m14F)
		m360 = 1;

	if (State == 0x40000000 && m14F)
		m360 = 1;

	if (m149 == -1)
		m360 = 1;
	else {
		if ((m368 & 0x4000) && Group[Trick].Frames &&
		    mFrame >= m14E - Group[Trick].w5 - 1)
			m360 = 1;

		if ((m368 & 0x2000) && mFrame >= m14E - Group[Trick].w5 - 1)
			m360 = 1;
	}

	if (m360) {
		FUN_8003159c(this);

		m364 = 1;
	}

	if (!m1EC)
		FUN_80031500(this);

	if (!m14F)
		return;

	State = m368 & 0xff000000;
	Trick = m368 & 0xff;

	switch (State) {
		case 0x40000000:
			if (m368 & 0x4000) {
				if (mPad->Circle.Pressed)
					m368 |= 0x800;

				if (!mPad->Circle.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2__6CBruceiiii(this, mAnim, Group[Trick].Frames, -1, 2529);
					else if (!m6DC) {
						m150 = 0x10000;
						Debug_RunAnim__6CBruceii(this, 14, 2535);

						m35C = 0;
					}
				}
			} else {
				if (mPad->Square.Pressed)
					m368 |= 0x800;

				if (!mPad->Square.Pressed)
					m368 &= ~0x800;

				if (!(m368 & 0x800)) {
					if (Group[Trick].Frames >= mFrame)
						Debug_RunAnim2__6CBruceiiii(this, mAnim, Group[Trick].Frames, -1, 2553);
					else if (!m6DC) {
						m150 = 0x10000;
						Debug_RunAnim__6CBruceii(this, 14, 2559);

						m35C = 0;
					}
				}
			}

			break;
		case 0x20000000:
			if (mPad->Circle.Pressed)
				m368 |= 0x800;

			if (!mPad->Circle.Pressed)
				m368 &= ~0x800;

			if (!(m368 & 0x800)) {
				if (m368 & 0x400) {
					if (!m6DC) {
						m150 = 0x10000;
						Debug_RunAnim__6CBruceii(this, 14, 2580);

						m35C = 0;
					}
				} else {
					Debug_RunAnim2__6CBruceiiii(this, mAnim, mFrame, 0, 2586);

					m368 |= 0x400;
				}
			}

			break;
		case (int)0x80000000:
			if (!m6DC) {
				m150 = 0x10000;
				Debug_RunAnim__6CBruceii(this, 14, 2596);

				m35C = 0;
			}

			break;
	}
}
