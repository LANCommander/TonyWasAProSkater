/* PLATFORM.CPP: CPlatform, the moving / bouncy / scalable script platforms (a CBaddy), and
   the skaters' bounces off them (declarations in game/platform.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/platform.h"
#include "game/bruce.h"
#include "game/colij.h"
#include "game/m3dasm.h"
#include "game/m3d.h"
#include "game/sfx.h"
#include "game/backgrnd.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/mech.h"
#include "game/shatter.h"
#include "game/spool.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/m3dzone.h"

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))
extern "C" void FUN_8003ec40(CPlatform *);
/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *);
};
struct Item;

/* __9CPlatformPsi (0x8003d6d8) */
/* A platform: a baddy using the current object file's models, on the
   environmental-object list, placed from its object data at node `node`, running its script. */
CPlatform::CPlatform(short *p_info, int node) {
	mpCreatedByPlayer = pCurrentPlayer;
	InitItem(pCurrentObjFile);
	AttachTo(&EnvironmentalObjectList);

	mType = 402;
	mFlags |= 0x111;
	mFlags &= ~2;
	p_info = SquirtPos(p_info);
	mpScript = (ushort *)SquirtAngles(p_info);
	mNode = node;
	mLoopingSoundUpdate = -1;
	mBaddyAttributes[0] = 32;
	mDoScript = 1;
}

/* _._9CPlatform (0x8003d7a0) */
/* Leaves the environmental-object list and stops the looping sound; CBaddy's
   destructor does the rest. */
CPlatform::~CPlatform() {
	DeleteFrom(&EnvironmentalObjectList);

	if (mLoopingSound) {
		SFX_Stop(mLoopingSound);

		mLoopingSound = 0;
	}
}

/* Bounce__9CPlatformP6CBruceRC7CVector (0x8003d820) */
/* Locals from the demo's MAIN.SYM. THPS1's CPlatform fields sit 8 bytes past
   the demo's; the CBaddy/CBody part is laid out by offset. */
void CPlatform::Bounce(CBruce *pSkater, const CVector &Direction) {
	int Height;
	int Speed;
	CVector Dir;

	if (mSpecificFlags & 0x10)
		if (!(mSpecificFlags & 0x20)) {
			Dir = Direction;
			Dir.vy = 0;
			Speed = Dir.Length();
			mSpecificFlags |= 0x20;
			mNumBounces = 0;
			mFlags |= 0x20;
			mTargetAngles.vy = Rnd(4096);
			mTargetAngles.vx = mAngles.vx - 1024 + (Rnd(2) << 11);
			mTargetAngles.vz = mAngles.vz - 1024 + (Rnd(2) << 11);

			switch (mType) {
				case 1:
					SFX_PlayX(104, 0x2000, 0);
					goto Bouncy;
				case 3:
					SFX_PlayX(40, 0x2000, 0);
				Bouncy:
					SFX_Grunt(pSkater);
					pSkater->Vibrate(6, 1, 120);

					Height = -0x40000;

					if (Speed < 40)
						Height = -0x20000;

					if (Speed < 18) {
						mTargetAngles.vy = mAngles.vy + Rnd(1024) - 512;
						mTargetAngles.vx = (mAngles.vx + 512) & 0xc00;
						mTargetAngles.vz = (mAngles.vz + 512) & 0xc00;
					}

					mVel.Set(Direction.vx, Height, Direction.vz);
					break;
				case 2:
				case 4:
				case 5:
					if (mType == 2)
						SFX_PlayX(GLevel ? 106 : Rnd(2) + 83, 0x2000, 0);
					else if (mType == 4) {
						SFX_Grunt(pSkater);
						SFX_PlayX(82, 0x2000, 0);
					}

					pSkater->Vibrate(2, 1, 180);
					mVel.Set(Direction.vx + (Direction.vx >> 1), -0x50000,
					         Direction.vz + (Direction.vz >> 1));

					if (ABS(mTargetAngles.vx) == 0x800 && mTargetAngles.vz == 0)
						mTargetAngles.vz = mAngles.vz;

					if (mTargetAngles.vx == 0 && ABS(mTargetAngles.vz) == 0x800)
						mTargetAngles.vx = mAngles.vx;

					break;
				default:
					mVel.Set(Direction.vx, -0x40000, Direction.vz);
					break;
			}
		}
}

/* BouncyAI__9CPlatform (0x8003db00) */
/* Fuzzy demo twin (opcode-identical but for a few words); locals and blocks from its
   MAIN.SYM. THPS1's CPlatform fields sit 8 bytes past the demo's; the CBaddy part is laid out by offset. */
void CPlatform::BouncyAI() {
	MATRIX m;
	SLineInfo LInfo;
	CVector NewPos;
	CVector NewVel;
	CFriction Fric;
	int ObjectExtent;

	if (mDeathTimer && !--mDeathTimer) {
		Die();
		return;
	}

	if (!(mSpecificFlags & 0x20))
		return;

	mTargetAngles.Mask();

	mAngVel.vx = (mTargetAngles.vx - mAngles.vx) >> 5;
	mAngVel.vy = (mTargetAngles.vy - mAngles.vy) >> 5;
	mAngVel.vz = (mTargetAngles.vz - mAngles.vz) >> 5;
	mAngVel.KillSmall();

	if (ABS(mAngVel.vx) < 40 && ABS(mTargetAngles.vx - mAngles.vx) > 40)
		mAngVel.vx = mAngVel.vx < 0 ? -40 : 40;

	if (ABS(mAngVel.vy) < 40 && ABS(mTargetAngles.vy - mAngles.vy) > 40)
		mAngVel.vy = mAngVel.vy < 0 ? -40 : 40;

	if (ABS(mAngVel.vz) < 40 && ABS(mTargetAngles.vz - mAngles.vz) > 40)
		mAngVel.vz = mAngVel.vz < 0 ? -40 : 40;

	mAngles += mAngVel;
	mAngles.Mask();
	M3dMaths_RotMatrixYXZ((SVECTOR *)&mAngles, &m);

	ObjectExtent =
	    (mWidth * ABS(m.m[1][0]) + mHeight * ABS(m.m[1][1]) + mDepth * ABS(m.m[1][2])) >> 1;
	NewVel = mVel;
	NewVel.vy += 0x8000;
	Fric = CFriction(6, 6, 6);
	NewVel %= Fric;
	NewVel.KillSmall();

	NewPos = mPos + NewVel;
	CVector Dir = NewVel;

	Dir.vy = 0;
	int Speed = Dir.Length();

	for (int NumChecks = 0; NumChecks < 3; NumChecks++) {
		LInfo.StartCoords.vx = mPos.vx;
		LInfo.StartCoords.vy = mPos.vy;
		LInfo.StartCoords.vz = mPos.vz;
		LInfo.EndCoords.vx = NewPos.vx;
		LInfo.EndCoords.vy = NewPos.vy;
		LInfo.EndCoords.vz = NewPos.vz;
		M3dColij_InitLineInfo(&LInfo);
		M3dZone_LineToItemNotSCached(&LInfo, 1);

		if (!LInfo.pItem) {
			if (NewVel.vy > 0) {
				LInfo.StartCoords.vx = NewPos.vx;
				LInfo.StartCoords.vy = NewPos.vy;
				LInfo.StartCoords.vz = NewPos.vz;
				LInfo.EndCoords = LInfo.StartCoords;
				LInfo.StartCoords.vy -= ObjectExtent;
				LInfo.EndCoords.vy += ObjectExtent;
				M3dColij_InitLineInfo(&LInfo);
				M3dZone_LineToItemNotSCached(&LInfo, 1);
			}

			if (!LInfo.pItem) {
				mPos = NewPos;
				mVel = NewVel;
				break;
			}
		}

		if (mNumBounces < 5) {
			switch (mType) {
				case 1:
					SFX_PlayX(!Rnd(2) ? 103 : 102, 0x2000, 0);
					break;
				case 3:
					SFX_PlayX(40, 0x2000, 0);
					break;
				case 2:
					if (GLevel)
						SFX_PlayX(!Rnd(2) ? 107 : 106, 0x2000, 0);
					else
						SFX_PlayX(Rnd(2) + 83, 0x2000, 0);

					break;
				case 4:
					SFX_PlayX(!Rnd(2) ? 81 : 80, 0x2000, 0);
					break;
				case 5:
					SFX_PlayX(Rnd(2) + 83, 0x2000, 0);
					break;
			}
		}

		mNumBounces++;

		{
			CVector Normal(LInfo.Normal.vx, LInfo.Normal.vy, LInfo.Normal.vz);
			int dot = Mips_CVecDotProd(&NewVel, &Normal);

			NewVel.vx -= Mips_FixMul(Normal.vx, dot) * 2;
			NewVel.vy -= Mips_FixMul(Normal.vy, dot) * 2;
			NewVel.vz -= Mips_FixMul(Normal.vz, dot) * 2;
			Fric = CFriction(3, 2, 3);

			if (ABS(NewVel.vy) < 0xc000) {
				Fric.vy = 0;
				Fric.vz = 2;
				Fric.vx = 2;
			} else {
				mTargetAngles.vy = Rnd(4096);

				if (mNumBounces == 1) {
					mTargetAngles.vx += (short)((Rnd(2) << 11) - 1024);

					if ((mType == 2 || mType == 4) &&
					    ((mTargetAngles.vx == 0x800 && mTargetAngles.vz == 0) ||
					     (mTargetAngles.vx == 0 && mTargetAngles.vz == 0x800)))
						mTargetAngles.vx = (Rnd(2) << 11) - 1024;
				}

				if (mNumBounces == 1) {
					mTargetAngles.vz += (Rnd(3) - 1) << 10;

					if ((mType == 2 || mType == 4) &&
					    ((mTargetAngles.vx == 0x800 && mTargetAngles.vz == 0) ||
					     (mTargetAngles.vx == 0 && mTargetAngles.vz == 0x800)))
						mTargetAngles.vz = (Rnd(2) << 11) - 1024;
				}

				if (mNumBounces == 1 && Speed < 18) {
					mTargetAngles.vy = mAngles.vy + Rnd(1024) - 512;
					mTargetAngles.vx = (mAngles.vx + 512) & 0xc00;
					mTargetAngles.vz = (mAngles.vz + 512) & 0xc00;
				}
			}

			NewVel %= Fric;
			NewVel.KillSmall();

			if (mNumBounces >= 19) {
				mAngles.vx = mTargetAngles.vx;
				mAngles.vz = mTargetAngles.vz;
				M3dMaths_RotMatrixYXZ((SVECTOR *)&mAngles, &m);

				NewVel.vz = 0;
				NewVel.vy = 0;
				NewVel.vx = 0;
				mDeathTimer = 10;
				mSpecificFlags &= ~0x20;
				return;
			}

			NewPos.vx = LInfo.Position.vx + Mips_FixMul(LInfo.Normal.vx, ObjectExtent);
			NewPos.vy = LInfo.Position.vy + Mips_FixMul(LInfo.Normal.vy, ObjectExtent);
			NewPos.vz = LInfo.Position.vz + Mips_FixMul(LInfo.Normal.vz, ObjectExtent);
		}
	}

	if (mNumBounces >= 19) {
		NewVel.vz = 0;
		NewVel.vy = 0;
		NewVel.vx = 0;
		mDeathTimer = 10;
		mAngles.vx = mTargetAngles.vx;
		mAngles.vz = mTargetAngles.vz;
		mSpecificFlags &= ~0x20;
	}
}

/* AI__9CPlatform (0x8003e390) */
/* : CPlatform::AI (vtable slot 2). */
void CPlatform::AI() {
	CSVector aim;
	CSVector ang;
	CVector *s3, *s2;
	unsigned short t, t2;
	int v, m, w;
	short a, b;

	t = mScaleXTimer;

	if (t) {
		mScaleXTimer = t - 1;
		mScale.vx = (ushort)mScale.vx + mScaleXAdjust;
	}

	t = mScaleYTimer;

	if (t) {
		mScaleYTimer = t - 1;
		mScale.vy = (ushort)mScale.vy + mScaleYAdjust;
	}

	t = mScaleZTimer;

	if (t) {
		mScaleZTimer = t - 1;
		mScale.vz = (ushort)mScale.vz + mScaleZAdjust;
	}

	mAcc.vz = 0;
	mAcc.vy = 0;
	mAcc.vx = 0;

	if (mTrodUpon == 0)
		mNotTrodUponTimer = mNotTrodUponTimer + 1;

	t = mInputFlags;
	mInputFlagsCopy = t;

	if (t & 1) {
		mInputFlags = t & 0xfffe;
		mInputSignal = mInputSignal + 1;
	}

	if (mSpecificFlags & 0x10) {
		BouncyAI();
		return;
	}

	if (mDoScript) {
		t2 = mWaitTimer;

		if (t2)
			mWaitTimer = t2 - 1;
		else
			ParseScript(mpScript);
	} else
		FUN_8003ec40(this);

	if (mSpecificFlags & 1) {
		s3 = &mPos;
		s2 = &mTargetPos;
		CVector v20 = *s3 - *s2;

		m = (mBaddyAttributes[0] >= v20.Length());

		if (m) {
			mPos = mTargetPos;

			if (mBaddyAttributes[2] < 2) {
				mVel.vz = 0;
				mVel.vy = 0;
				mVel.vx = 0;
			}

			mSpecificFlags = mSpecificFlags & ~1;
		} else {
			t = mBaddyAttributes[2];

			if (t == 0) {
				Utils_CalcAim(aim, *s3, *s2);
				Utils_GetVecFromMagDir(mVel, mBaddyAttributes[0], aim);
			} else if (t == 1) {
				Utils_CalcAim(aim, *s3, *s2);
				CVector v38 = *s2 - *s3;

				v = 8160 / v38.Length();

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				ang = mAngles;
				Utils_TurnTowards(ang, mAngVel, mAngAcc, aim, v);

				mAngles.vx = 0;
				Utils_GetVecFromMagDir(mAcc, mBaddyAttributes[0], aim);

				a = 0;

				if (mSpecificFlags & 0x20)
					a = ((unsigned short)aim.vy - (ushort)mAngles.vy) & 0xfff;

				if (a > 0x800)
					a = a - 0x1000;

				if (a > 0x400)
					a = 0x400;
				else if (a < -0x400)
					a = -0x400;

				w = -a;
				a = (w >> 1) & 0xfff;
				b = a - (ushort)mAngles.vz;

				if (b < -0x800)
					b += 0x1000;

				if (b > 0x800)
					b -= 0x1000;

				if (b != 0)
					mAngAcc.vz = b >> 3;
				else
					mAngVel.vz = 0;
			} else if (t == 2) {
				Utils_CalcAim(aim, *s3, *s2);
				CVector v48 = *s2 - *s3;

				v = 8160 / v48.Length();

				if (v < -256)
					v = -256;
				else if (v > 256)
					v = 256;
				else if ((unsigned)(v - 1) < 3)
					v = 4;
				else if (v < 0) {
					if (v >= -3)
						v = -4;
				}

				ang = mAngles;
				Utils_TurnTowards(ang, mAngVel, mAngAcc, aim, v);

				mAngles.vx = 0;
				Utils_GetVecFromMagDir(mAcc, mBaddyAttributes[0], aim);
			}
		}
	} else if (mBaddyAttributes[2] == 1) {
		a = -(ushort)mAngles.vz;

		if (a < -0x800)
			a += 0x1000;

		if (a > 0x800)
			a -= 0x1000;

		if (a == 0)
			mAngVel.vz = 0;
		else
			mAngAcc.vz = a >> 3;
	}

	if (mApplyGravity)
		mAcc += DAT_800a7f28;

	DoPhysics();

	if (mLoopingSoundUpdate != -1 && TTime % mLoopingSoundUpdate == 0)
		SFX_ModifyPos(mLoopingSound, mPos, 0);

	mTrodUpon = 0;
}

/* FUN_8003e934 (0x8003e934) */
/* CPlatform::Hit (vtable slot 5), kept as the function retail has: counts the hit; a breakable
   platform (mBaddyAttributes[1]) loses health, and dies (CBody::Die, slot 1) and shatters when it
   runs out, else flashes white. */
extern "C" int FUN_8003e934(CPlatform *p, int a) {
	p->mHit = p->mHit + 1;

	if (p->mBaddyAttributes[1]) {
		if (p->mHealth > 0) {
			p->mHealth = (ushort)p->mHealth - a;

			if (p->mHealth <= 0) {
				struct VtEntry *vt = (struct VtEntry *)p->__vptr;

				vt[2].fn((char *)p + vt[2].delta);
				Shatter_Item((Item *)p, 0, 1);
			} else
				p->Flash(3, 255, 255, 255);
		}
	}

	return 1;
}

/* DoPhysics__9CPlatform (0x8003e9d8) */
void CPlatform::DoPhysics() {
	mNormal.vz = 0;
	mNormal.vy = 0;
	mNormal.vx = 0;

	if (mBaddyAttributes[2] == 0) {
		int vx, vy, vz;
		short sx, sy, sz;

		vx = mVel.vx + mAcc.vx;
		vy = mVel.vy + mAcc.vy;
		vz = mVel.vz + mAcc.vz;

		if (vx >= -2048 && vx <= 2048)
			vx = 0;

		if (vy >= -2048 && vy <= 2048)
			vy = 0;

		if (vz >= -2048 && vz <= 2048)
			vz = 0;

		mVel.vx = vx;
		mVel.vy = vy;
		mVel.vz = vz;
		mPos.vx += vx;
		mPos.vy += vy;
		mPos.vz += vz;
		sx = mAngVel.vx;
		sy = mAngVel.vy;
		sz = mAngVel.vz;
		mAngles.vx = (mAngles.vx + sx) & 4095;
		mAngles.vy = (mAngles.vy + sy) & 4095;
		mAngles.vz = (mAngles.vz + sz) & 4095;
		sx = sx + mAngAcc.vx;
		sy = sy + mAngAcc.vy;
		sz = sz + mAngAcc.vz;

		if (sx >= -1 && sx <= 1)
			sx = 0;

		if (sy >= -1 && sy <= 1)
			sy = 0;

		if (sz >= -1 && sz <= 1)
			sz = 0;

		mAngVel.vx = sx;
		mAngVel.vy = sy;
		mAngVel.vz = sz;
	} else if (mBaddyAttributes[2] == 1 || mBaddyAttributes[2] == 2) {
		mVel += mAcc;
		mVel %= mFric;
		mVel.KillSmall();

		mPos += mVel;
		mAngles += mAngVel;
		mAngles.Mask();

		mAngVel += mAngAcc;
		mAngVel %= mAngFric;
		mAngVel.KillSmall();
	}
}

/* MoveTo__9CPlatformR7CVector (0x8003ebdc) */
/* Sets a new target position; flags the platform as moving if it changed. */
void CPlatform::MoveTo(CVector &pos) {
	if (mTargetPos != pos) {
		mTargetPos = pos;
		mSpecificFlags |= 1;
	}
}

/* FUN_8003ec40 (0x8003ec40) */
void FUN_8003ec40(void) {}

/* ExecuteCommand__9CPlatformUs (0x8003ec48) */
/* Fuzzy demo twin (0.99); locals from its MAIN.SYM. Platform script commands:
   bounding size, visibility, looping sounds, scale and spin rates per axis, collision radius,
   moves to absolute/relative/node positions, and shattering. Anything else goes to the baddy
   command handler. */
int CPlatform::ExecuteCommand(ushort command) {
	switch (command) {
		case 17161: {
			SModel *m;
			ushort *pBox;
			ushort v = *mpScript++;

			mType = v;

			if (v)
				mSpecificFlags |= 16;
			else
				mSpecificFlags &= ~16;

			m = PSXRegion[mRegion].ppModels[mModel];
			pBox = (ushort *)&m->Box;
			mWidth = pBox[0] - pBox[1];
			mHeight = pBox[2] - pBox[3];
			mDepth = pBox[4] - pBox[5];
			break;
		}
		case 17157:
			if (*mpScript++)
				mFlags |= 8;
			else
				mFlags &= ~8;

			break;
		case 17671:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = SFX_PlayX(*mpScript++, 0x2000, 0);
			mLoopingSoundUpdate = -1;
			break;
		case 17672: {
			int id;

			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			id = *mpScript++;
			mLoopingSoundUpdate = *mpScript++;
			mLoopingSound = SFX_PlayPos(id, mPos, 0);
			break;
		}
		case 17673:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = 0;
			break;
		case 17156: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleXTimer = mScaleYTimer = mScaleZTimer = *mpScript++;

			if (mScaleXTimer)
				mScaleYAdjust = mScaleZAdjust = mScaleXAdjust = (p << 12) / (mScaleXTimer * 100);
			else
				mScaleYAdjust = mScaleZAdjust = mScaleXAdjust = (p << 12) / 100;

			mScale.vx += mScaleXAdjust;
			mScale.vy += mScaleYAdjust;
			mScale.vz += mScaleZAdjust;
			break;
		}
		case 17158: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleXTimer = *mpScript++;

			if (mScaleXTimer)
				mScaleXAdjust = (p << 12) / (mScaleXTimer * 100);
			else
				mScaleXAdjust = (p << 12) / 100;

			mScale.vx += mScaleXAdjust;
			break;
		}
		case 17159: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleYTimer = *mpScript++;

			if (mScaleYTimer)
				mScaleYAdjust = (p << 12) / (mScaleYTimer * 100);
			else
				mScaleYAdjust = (p << 12) / 100;

			mScale.vy += mScaleYAdjust;
			break;
		}
		case 17160: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleZTimer = *mpScript++;

			if (mScaleZTimer)
				mScaleZAdjust = (p << 12) / (mScaleZTimer * 100);
			else
				mScaleZAdjust = (p << 12) / 100;

			mScale.vz += mScaleZAdjust;
			break;
		}
		case 17155:
			mBruceHealthAdjust = *mpScript++;
			break;
		case 17154: {
			int x = *mpScript++;
			int y = *mpScript++;
			int z = *mpScript++;

			mSpecificFlags |= 4;
			mSpin[0] = x << 11;
			mSpin[1] = y << 11;
			mSpin[2] = z << 11;
			break;
		}
		case 17152:
			if (mTrodUpon)
				return 1;

			mpScript--;
			return 0;
		case 16928: {
			int *p_pos;
			CVector pos;

			p_pos = (int *)(((intptr_t)mpScript + 3) & ~3);
			pos.vx = *p_pos++;
			pos.vy = *p_pos++;
			pos.vz = *p_pos++;
			pos <<= 12;
			mpScript = (ushort *)p_pos;
			MoveTo(pos);
			break;
		}
		case 16929: {
			ushort node = *mpScript++;
			CVector pos;

			if (node & 0x2000)
				node = GetVariable(node);

			Trig_GetPosition(pos, node);
			MoveTo(pos);
			return 1;
		}
		case 16930: {
			int *p_pos;
			CVector pos;

			p_pos = (int *)(((intptr_t)mpScript + 3) & ~3);
			pos.vx = *p_pos++;
			pos.vy = *p_pos++;
			pos.vz = *p_pos++;
			pos <<= 12;
			pos += mPos;
			mpScript = (ushort *)p_pos;
			MoveTo(pos);
			break;
		}
		case 16901:
			Die();
			return 0;
		case 17153:
			if (MechList->m868 == this)
				MechList->m868 = 0;

			mFlags |= 1;
			Die();
			Shatter_Item((Item *)this, 0, 0);
			return 0;
		default:
			return CBaddy::ExecuteCommand(command);
	}

	return 1;
}

/* SetVariable__9CPlatformUs (0x8003f45c) */
void CPlatform::SetVariable(ushort variable) {
	switch (variable) {
		case 8483:
			mApplyGravity = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8484:
			mModel = *mpScript++;
			break;
		case 8495: {
			uint *p_checksum = (uint *)(((intptr_t)mpScript + 3) & -4);

			mModel = Spool_GetModel(*p_checksum, mRegion);
			mpScript = (ushort *)(p_checksum + 1);
			mFlags &= ~1;
			break;
		}
		case 8500: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mVel.Set(a << 12, b << 12, c << 12);
			break;
		}
		case 8503: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngles.Set(a, b, c);
			break;
		}
		case 8487: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngVel.Set(a, b, c);
			break;
		}
		case 8488: {
			short a = GetScriptValue();
			short b = GetScriptValue();
			short c = GetScriptValue();

			mAngAcc.Set(a, b, c);
			break;
		}
		default:
			CBaddy::SetVariable(variable);
			break;
	}
}

/* GetVariable__9CPlatformUs (0x8003f624) */
/* Variable 0x2200 is the platform's not-trodden-upon timer (low half, signed);
   the rest are CBaddy's. */
short CPlatform::GetVariable(ushort variable) {
	if (variable == 0x2200)
		return (short)mNotTrodUponTimer;

	return CBaddy::GetVariable(variable);
}

/* Platform_HandleBounce__Fv (0x8003f65c) */
/* Fuzzy demo twin (0.52; the demo takes the skater as a parameter); locals from
   its MAIN.SYM. Builds each skater's swept box (old to new position, padded by 0x14000) and bounces
   them off every active bouncy platform whose model box it overlaps. THPS1 checks the second skater
   too when two are playing. */
void Platform_HandleBounce() {
	Item *pItem;
	int tmp;
	int LP1, RP1, FP1, BP1, UP1, DP1;
	int LP2, RP2, FP2, BP2, UP2, DP2;
	CBruce *pSkater2;

	LP1 = MechList->mOldPos.vx;
	RP1 = MechList->mPos.vx;
	pSkater2 = (CBruce *)MechList->mNextItem;

	if (RP1 < LP1) {
		tmp = LP1;
		LP1 = RP1;
		RP1 = tmp;
	}

	LP1 -= 0x14000;
	RP1 += 0x14000;
	UP1 = MechList->mOldPos.vy;
	DP1 = MechList->mPos.vy;

	if (DP1 < UP1) {
		tmp = UP1;
		UP1 = DP1;
		DP1 = tmp;
	}

	UP1 -= 0x14000;
	DP1 += 0x14000;
	FP1 = MechList->mOldPos.vz;
	BP1 = MechList->mPos.vz;

	if (BP1 < FP1) {
		tmp = FP1;
		FP1 = BP1;
		BP1 = tmp;
	}

	FP1 -= 0x14000;
	BP1 += 0x14000;

	if (NumMechs >= 2) {
		LP2 = pSkater2->mOldPos.vx;
		RP2 = pSkater2->mPos.vx;

		if (RP2 < LP2) {
			tmp = LP2;
			LP2 = RP2;
			RP2 = tmp;
		}

		LP2 -= 0x14000;
		RP2 += 0x14000;
		UP2 = pSkater2->mOldPos.vy;
		DP2 = pSkater2->mPos.vy;

		if (DP2 < UP2) {
			tmp = UP2;
			UP2 = DP2;
			DP2 = tmp;
		}

		UP2 -= 0x14000;
		DP2 += 0x14000;
		FP2 = pSkater2->mOldPos.vz;
		BP2 = pSkater2->mPos.vz;

		if (BP2 < FP2) {
			tmp = FP2;
			FP2 = BP2;
			BP2 = tmp;
		}

		FP2 -= 0x14000;
		BP2 += 0x14000;
	}

	for (pItem = (Item *)EnvironmentalObjectList; pItem; pItem = pItem->NextItem) {
		if (pItem->Flags & 0x100) {
			if (((CPlatform *)pItem)->mSpecificFlags & 16) {
				pItem->Flags |= 32;

				if (!(((CPlatform *)pItem)->mSpecificFlags & 32)) {
					SModel *m = PSXRegion[pItem->Region].ppModels[pItem->Model];
					short *pBox = (short *)((char *)m + 12);

					if (RP1 >= pItem->Position.vx + (pBox[1] << 12) &&
					    pItem->Position.vx + (pBox[0] << 12) >= LP1 &&
					    BP1 >= pItem->Position.vz + (pBox[5] << 12) &&
					    pItem->Position.vz + (pBox[4] << 12) >= FP1 &&
					    DP1 >= pItem->Position.vy + (pBox[3] << 12) &&
					    pItem->Position.vy + (pBox[2] << 12) >= UP1)
						((CPlatform *)pItem)->Bounce(MechList, MechList->mVel);
					else if (NumMechs >= 2 && RP2 >= pItem->Position.vx + (pBox[1] << 12) &&
					         pItem->Position.vx + (pBox[0] << 12) >= LP2 &&
					         BP2 >= pItem->Position.vz + (pBox[5] << 12) &&
					         pItem->Position.vz + (pBox[4] << 12) >= FP2 &&
					         DP2 >= pItem->Position.vy + (pBox[3] << 12) &&
					         pItem->Position.vy + (pBox[2] << 12) >= UP2)
						((CPlatform *)pItem)->Bounce(pSkater2, pSkater2->mVel);
				}
			}
		}
	}
}
