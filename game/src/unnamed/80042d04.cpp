/* FUN_80042d04 (0x80042d04) */
/* C++ (MECH.CPP): CBruce::AI (vtable slot 2). The fields at +0x890, +0x8fc/+0x8fd and +0x91c have
   no names in bruce.h yet: reached through its padding arrays. */
#include "game/bruce.h"
#include "game/camera.h"
#include "game/career.h"
#include "game/powerup.h"
#include "game/video.h"
#include "game/psyq.h"

extern int GamePaused;
extern CCamera *CameraList;
extern int XFrames;
extern CBody *BaddyList;
extern CBody *PowerUpList;
extern int Cheat_FPS;
extern int Cheat_StatsOverride;
extern CBruce *GSkater;
extern int Cheat_Fire;
extern SVideoInfo GVInfo1[2];
extern char DAT_800d25f0[];
extern SStats Stats[];

extern CBruce *MechList[2];

void Video_Record(CBruce *);
void Video_Playback(CBruce *);
extern "C" void FUN_80042708(CBruce *);
extern "C" void FUN_80042728(CBruce *);
CBody *M3dColij_LineToSphere(const CVector &, const CVector &, CVector &, CBody *, CBody *); /* ScaleFactor unused */
extern "C" void FUN_800427cc(CBruce *, CBody *);
void Font_String(ushort, ushort, char *, uchar, uchar, uchar, uchar);

#define CC (mBalanceFlash || Cheat_Fire)

#define M890 (pad88c[0x890 - 0x88c])        /* +0x890 */
#define M8FC (pad8fc[0])                    /* +0x8fc */
#define M8FD (pad8fc[1])                    /* +0x8fd */
#define M91C (*(int *)&pad91a[0x91c - 0x91a]) /* +0x91c */

void CBruce::AI() {
	CVector sp20;
	int n2, n3, n4, n5, sx, sy, u, w, w2, w3, x1, x2, x3, x4, x5, x6, x7, d, t0, a2, a3, v2, e;
	SVideoInfo *pp;
	CVector *s1;
	CVector *s2;
	CBody *b;
	char *p1;
	char *p2;

	if (GamePaused == 0) {
		if (CameraList->InCompetitionIntro()) {
			mTime = XFrames + 0xe2e;
			return;
		}

		if (M890 == 0) {
			x1 = m310;

			if (x1 && XFrames - x1 >= 11)
				m310 = 0;

			x2 = m314;

			if (x2 && XFrames - x2 >= 11)
				m314 = 0;

			if (this == MechList[0])
				pp = &GVInfo1[0];
			else
				pp = &GVInfo1[1];

			x3 = pp->VideoMode;

			switch (x3) {
				case 1:
					Video_Record(this);
					break;
				case 2:
					Video_Playback(this);
					return;
			}

			{
				m6F0 = 100;
				mRGB = 0x404040;
				FUN_80042708(this);
				DoPhysics();
				HandleWater();

				if (((mCollision >> 1) ^ 1) & 1)
					m8FE = m8FE + 1;
				else {
					m8FE = 0;
					M8FD = 0;
				}

				if ((uchar)mAX | (uchar)mAY) {
					x4 = M91C;

					if (x4 < 8)
						M91C = x4 + 1;
				} else {
					x5 = M91C;

					if (x5)
						M91C = x5 - 1;
				}

				FUN_80042728(this);

				if ((uchar)M8FC)
					M8FC = (uchar)M8FC - 1;

				s2 = &mOldPos;
				s1 = &mPos;
				b = M3dColij_LineToSphere(*s2, *s1, sp20, BaddyList, 0);

				if (b)
					FUN_800427cc(this, b);

				b = M3dColij_LineToSphere(*s2, *s1, sp20, PowerUpList, 0);

				if (b)
					((CPowerUp *)b)->TakeEffect(this);

				if (mPhysicsState || (d = mPos.vy - m2E4, ((d ^ (d >> 31)) - (d >> 31)) >= 65)) {
					m2E4 = mPos.vy;
					DoShadowCheck();
				} else {
					mShadowPos.vx = mPos.vx;
					mShadowPos.vz = mPos.vz;
				}

				GlowUpdate();

				if (mHealth < 0)
					mHealth = 0;

				if (Stats[mSkaterIndex].ComboID == 0x4000) {
					if (mAnim != 0x3f)
						mSubObjectDisplayMask = mSubObjectDisplayMask | 0x1000;
					else
						mSubObjectDisplayMask = mSubObjectDisplayMask & ~0x1000;
				}

				if ((Cheat_FPS || Cheat_StatsOverride) && this == GSkater) {
					a2 = Cheat_StatsOverride;
					t0 = a2;
					p1 = DAT_800d25f0;
					p2 = (char *)"%d %d %d %d";

					if (a2 == 0) {
						e = Stats[mSkaterIndex].Stats[0];
						a2 = mpStatBonus[0];

						if (CC) {
							w = a2 + 3;
							a2 = e + w;
						} else {
							n2 = e + a2;
							a2 = n2;
						}
					}

					a3 = t0;

					if (a3 == 0) {
						e = Stats[mSkaterIndex].Stats[1];
						a3 = mpStatBonus[1];

						if (CC) {
							w2 = a3 + 3;
							a3 = e + w2;
						} else {
							n3 = e + a3;
							a3 = n3;
						}
					}

					sprintf(p1, p2, a2, a3, t0 ? t0 : ({
						        int e2 = Stats[mSkaterIndex].Stats[2];
						        int u2 = mpStatBonus[2];
						        int r2;

						        if (CC) {
							        r2 = u2 + 3;
							        n4 = e2 + r2;
							        r2 = n4;
						        } else
							        r2 = e2 + u2;

						        r2;
					        }),
					        Cheat_StatsOverride ? Cheat_StatsOverride : ({
						        int e3 = Stats[mSkaterIndex].Stats[3];
						        int u3 = mpStatBonus[3];
						        int r3;

						        if (CC) {
							        r3 = u3 + 3;
							        n5 = e3 + r3;
							        r3 = n5;
						        } else
							        r3 = e3 + u3;

						        r3;
					        }));
					Font_String(420, 200, DAT_800d25f0, 90, 90, 100, 0);
				}
			}
		}
	}
}
