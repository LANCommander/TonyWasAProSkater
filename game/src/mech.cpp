/* MECH.CPP: the skater object, CBruce (Neversoft "Bruce", a CSuper): construction and
   destruction, stats, glow and dynamic lighting, shadow, blood, water and its AI (declarations
   in game/bruce.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/mech.h"
#include "game/bruce.h"
#include "game/vector.h"
#include "game/colij.h"
#include "game/career.h"
#include "game/m3d.h"
#include "game/main.h"
#include "game/m3dasm.h"
#include "game/camera.h"
#include "game/pad.h"
#include "game/bit.h"
#include "game/powerup.h"
#include "game/video.h"
#include "game/psyq.h"
#include "game/trig.h"
#include "game/sfx.h"
#include "game/exp.h"
#include "game/font.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/m3dutils.h"
#include "game/physics.h"
#include "game/utils.h"

extern int DAT_800a8118[];
#define GetStat(n)                                                                                 \
	(Cheat_StatsOverride                                                                           \
	     ? Cheat_StatsOverride                                                                     \
		 : (Stats[mSkaterIndex].Stats[n] + ((mBalanceFlash || Cheat_Fire) ? 3 : 0)) +              \
		       (mpStatBonus ? mpStatBonus[n] : 0))
extern "C" char __vt_6CBruce[];
extern "C" void FUN_800427bc(CBruce *p);
extern "C" void FUN_8001dfec(FallingTextEffect *p, int a);
/* egcs-ABI calls, as retail makes them */
extern "C" void dt_6CSuper(CSuper *p, int a);
extern "C" void __dl__5CItemPv(void *p);
int GlowTowards(int, int, int);
extern int DAT_800d19ac;
int M3dZone_LineToItem(SLineInfo *, int);
extern SHooksPacket SkaterHooks[];
extern int Cheat_Blood;
extern char DAT_800d25f0[];
extern "C" void FUN_80042708(CBruce *);
extern "C" void FUN_80042728(CBruce *);
/* ScaleFactor unused */
extern "C" void FUN_800427cc(CBruce *, CBody *);
#define CC (mBalanceFlash || Cheat_Fire)
#define M890 (pad88c[0x890 - 0x88c])        /* +0x890 */
#define M8FC (pad8fc[0])                    /* +0x8fc */
#define M8FD (pad8fc[1])                    /* +0x8fd */
#define M91C (*(int *)&pad91a[0x91c - 0x91a]) /* +0x91c */

/* BoardOff__6CBruce (0x80041c30) */
/* Hides the board and both wheel models of this skater. */
void CBruce::BoardOff() {
	SStats *pStats = &Stats[mSkaterIndex];

	mSubObjectDisplayMask |= (1 << pStats->Board) | (1 << pStats->Wheel1) | (1 << pStats->Wheel2);
}

/* BoardOn__6CBruce (0x80041c88) */
/* Shows the board and both wheel models of this skater again. */
void CBruce::BoardOn() {
	SStats *pStats = &Stats[mSkaterIndex];

	mSubObjectDisplayMask &=
	    ~((1 << pStats->Board) | (1 << pStats->Wheel1) | (1 << pStats->Wheel2));
}

/* GetStats__6CBrucei (0x80041ce0) */
/* Demo twin; locals from its MAIN.SYM. Fills player Skater's four stat bytes: in a
   career game, from the best board the skater's tape count has unlocked; otherwise all 3. */
void CBruce::GetStats(int Skater) {
	if (GNumberOfPlayers == 1) {
		int highest_board = 0;

		while (highest_board < 7) {
			if (Career_Progress(mSkaterIndex)->NumTapes <
			    Stats[mSkaterIndex].Boards[highest_board + 1].Req)
				break;

			highest_board++;
		}

		GBoardStats[Skater].s[0] = Stats[mSkaterIndex].Boards[highest_board].Stats[0];
		GBoardStats[Skater].s[1] = Stats[mSkaterIndex].Boards[highest_board].Stats[1];
		GBoardStats[Skater].s[2] = Stats[mSkaterIndex].Boards[highest_board].Stats[2];
		GBoardStats[Skater].s[3] = Stats[mSkaterIndex].Boards[highest_board].Stats[3];
	} else {
		GBoardStats[Skater].s[0] = 3;
		GBoardStats[Skater].s[1] = 3;
		GBoardStats[Skater].s[2] = 3;
		GBoardStats[Skater].s[3] = 3;
	}
}

/* __6CBruce (0x80041e5c) */
/* Demo twin; statement order from its line table. The skater constructor: an
   inline CPanelData ctor (retail has it inline, demo out of line), then the per-level/game-mode
   defaults, the two kick speeds from stat 1, the identity-ish rotation, list attach and
   shadow setup. */
CBruce::CBruce() : mPanel(this) {
	int i;

	mAnimSpeed = 0x10000;

	if (Levels[GLevel].isCompetition && GGame == 1)
		mTime = 3630;
	else if (GGame != 7)
		mTime = 7230;
	else
		mTime = 630;

	mGravity.Set(0, 1000, 0);

	mFlags |= 0x200;
	mScale.Set(4096, 4096, 4096);

	mMaxSpeed = ((GetStat(1) + 39) << 12) * 3 * 15 / 28;
	mKickSpeed = ((GetStat(1) + 17) << 12) * 3 * 15 / 28;
	m340 = 30;
	m344 = 30;
	m6DC = 0;
	mStickToRail = 0;
	m70C = 0;
	mGrindSnd = 0;
	m710 = -1;
	mRollSnd = 0;
	mPhysicsState = PHYSICS_IN_AIR;
	mOldPhysicsState = PHYSICS_IN_AIR;
	m6A8 = 0;
	mManIsCrouched = 0;
	m740 = 0;
	mJumpPending = 0;
	mDoingATrick = 0;
	m750 = 0;
	m424 = 0;
	mExtraFlags |= 1;

	for (i = 0; i < TRICK_COUNT; i++)
		m428[i] = 0;

	for (i = 0; i < TRICK_COUNT; i++)
		m568[i] = 0;

	m764 = 0;
	m768 = 0;
	mFallLine.Set(0, 0, 0);

	mFallLineTurn = 0;
	mPolyNormal.vx = 0;
	mPolyNormal.vy = -4096;
	mPolyNormal.vz = 0;
	mPitchAngle = 0;
	mLeanAngle = 0;
	mAnimLeanAngle = 0;
	m840 = 0;
	mRotMatrix.m[0][0] = -4096;
	mRotMatrix.m[1][0] = 0;
	mRotMatrix.m[2][0] = 0;
	mRotMatrix.m[0][1] = 0;
	mRotMatrix.m[1][1] = -4096;
	mRotMatrix.m[2][1] = 0;
	mRotMatrix.m[0][2] = 0;
	mRotMatrix.m[1][2] = 0;
	mRotMatrix.m[2][2] = -4096;
	FRUVectorsFromRotMatrix();

	mOldRotMatrix = mRotMatrix;
	int x = mRotMatrix.m[0][1];
	int y = mRotMatrix.m[1][1];
	int z = mRotMatrix.m[2][1];

	mSameNormal = 0;
	mOldNormal.vx = x;
	mOldNormal.vy = y;
	mOldNormal.vz = z;
	m98 = 2;
	mNormal.vx = x;
	mNormal.vy = y;
	mNormal.vz = z;
	AttachTo((CBody **)&MechList);

	NumMechs++;
	mCBodyFlags &= ~2;

	if (NumMechs == 1)
		mpPad = Pad;
	else
		mpPad = &Pad[1];

	mAngFric.Set(5, 1, 5);

	if (GNumberOfPlayers == 1) {
		mShadowLights = 1;
		mpShadowSource = (SVECTOR *)Mem_New(32, 0, 1);
		mpShadowRGB = (CVECTOR *)Mem_New(16, 0, 1);
		((short *)mpShadowSource)[0] = 0;
		((short *)mpShadowSource)[1] = -4096;
		((short *)mpShadowSource)[2] = 0;
		((uchar *)mpShadowRGB)[0] = 128;
		((uchar *)mpShadowRGB)[1] = 128;
		((uchar *)mpShadowRGB)[2] = 128;
	} else {
		ShadowOn();
		UpdateShadow();
	}

	/* (no camera in the front end's skater preview: retail wrote into low RAM) */
	if (CameraList)
		CameraList->mCameraMode = CAMERAMODE_NORMAL;

	mpLight = (SLight *)DAT_800a8118;
}

/* FUN_80042380 (0x80042380) */
/* : CBruce's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags)
   retail has: stops the grind/roll sounds, frees the shadow buffers, leaves MechList and destroys
   the panel's two FallingTextEffects before CSuper's destructor. */
extern "C" void FUN_80042380(CBruce *p, int f) {
	p->__vptr = __vt_6CBruce;

	if (p->mGrindSnd) {
		SFX_Stop(p->mGrindSnd);

		p->mGrindSnd = 0;
	}

	if (p->mRollSnd) {
		SFX_Stop(p->mRollSnd);

		p->mRollSnd = 0;
	}

	if (p->mpShadowSource)
		Mem_Delete(p->mpShadowSource);

	if (p->mpShadowRGB)
		Mem_Delete(p->mpShadowRGB);

	p->DeleteFrom((CBody **)&MechList);

	NumMechs--;
	FUN_800427bc(p);
	FUN_8001dfec(&p->mPanel.mOutOfBoundsText, 2);
	FUN_8001dfec(&p->mPanel.mFallingTextEffect, 2);
	dt_6CSuper(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

/* GlowTowards__Fiii (0x80042474) */
int GlowTowards(int a, int b, int s) {
	if (a < b) {
		if (a + s > b)
			return b;

		return a + s;
	}

	if (a - s < b)
		return b;

	return a - s;
}

/* GlowUpdate__6CBruce (0x800424b4) */
/* Fades the skater's glow towards its target colour; once there, fades back to
   black at the down speed. A lit glow shows as the body's ellipsoid. */
void CBruce::GlowUpdate() {
	mGlowR = GlowTowards(mGlowR, mTargetGlowR, mGlowSpeed);
	mGlowG = GlowTowards(mGlowG, mTargetGlowG, mGlowSpeed);
	mGlowB = GlowTowards(mGlowB, mTargetGlowB, mGlowSpeed);

	if (mGlowR == mTargetGlowR && mGlowG == mTargetGlowG && mGlowB == mTargetGlowB) {
		if (mGlowDownSpeed) {
			mTargetGlowR = 0;
			mTargetGlowG = 0;
			mTargetGlowB = 0;
			mGlowSpeed = mGlowDownSpeed;
			mGlowDownSpeed = 0;
		}
	}

	if (mGlowR | mGlowG | mGlowB) {
		mEllipsoidScale = 0x2000;
		mEllipsoidOTPushback = 10;
		mEllipsoidRGB = 0x2000000 | mGlowR | (mGlowG << 8) | (mGlowB << 16);
	} else {
		mEllipsoidRGB = 0;
	}
}

/* FUN_800425b8 (0x800425b8) */
void FUN_800425b8(void) {}

/* CalculateDynamicLighting__6CBrucePUiP4Item (0x800425c0) */
void CBruce::CalculateDynamicLighting(uint *p_face, Item *p_item) {
	unsigned short f = *(unsigned short *)p_face;

	if (f & 0x800) {
		uint *pColourTable = PSXRegion[p_item->Region].pColourTable;
		int vertices = 4;

		mBrightness = 0;

		if (f & 0x10)
			vertices = 3;

		{
			int i;

			for (i = 0; i < vertices; i++) {
				uint rgb = pColourTable[((unsigned char *)p_face)[8 + i]] & 0xffffff;

				mBrightness = mBrightness + (rgb & 255) + ((rgb >> 8) & 255) + (rgb >> 16);
			}
		}

		mBrightness = mBrightness / (vertices * 3);
	} else {
		uint rgb = p_face[2] & 0xffffff;

		mBrightness = (int)((rgb & 255) + ((rgb >> 8) & 255) + (rgb >> 16)) / 3;
	}

	mBrightness = mBrightness * 3;
	DoLightingSetup();
}

/* SetGlow__6CBruceiiii (0x80042700) */
/* Empty in retail. */
void CBruce::SetGlow(int, int, int, int) {}

/* FUN_80042708 (0x80042708) */
/* C++: a skater's analogue stick from its pad. */
extern "C" void FUN_80042708(CBruce *p) {
	SControl *q = p->mpPad;

	p->mAY = q->AnalogueMoveForwardsBackwards;
	p->mAX = q->AnalogueMoveLeftRight;
}

/* FUN_80042728 (0x80042728) */
void FUN_80042728(void) {
	if (DAT_800d19ac)
		DAT_800d19ac--;
}

/* CanCollect__6CBruces (0x80042744) */
/* The skater can always collect. */
int CBruce::CanCollect(short) {
	return 1;
}

/* GetHookPosition__6CBruceR7CVectorUc (0x8004274c) */
/* The world position of hook point Index on the skater's model. */
void CBruce::GetHookPosition(CVector &Pos, uchar Index) {
	M3dUtils_GetHookPosition((VECTOR *)&Pos, this, Index);
}

/* Die__6CBruce (0x80042778) */
/* Kills the skater unless already dead: flags the body and item as dead. */
void CBruce::Die() {
	if (!IsDead()) {
		mCBodyFlags |= 0x40;
		mFlags |= 0x41;
	}
}

/* FUN_800427bc (0x800427bc) */
void FUN_800427bc(void) {}

/* FUN_800427c4 (0x800427c4) */
int FUN_800427c4(void) {
	return 1;
}

/* FUN_800427cc (0x800427cc) */
void FUN_800427cc(void) {}

/* DoShadowCheck__6CBruce (0x800427d4) */
void CBruce::DoShadowCheck() {
	SLineInfo line_info;

	FRUVectorsFromRotMatrix();

	if (GNumberOfPlayers == 1) {
		line_info.StartCoords.vx = mPos.vx + mUp.vx * 50;
		line_info.StartCoords.vy = mPos.vy + mUp.vy * 50;
		line_info.StartCoords.vz = mPos.vz + mUp.vz * 50;
	} else {
		line_info.StartCoords.vx = mPos.vx;
		line_info.StartCoords.vy = mPos.vy;
		line_info.StartCoords.vz = mPos.vz;
	}

	line_info.EndCoords.vx = line_info.StartCoords.vx;
	line_info.EndCoords.vy = line_info.StartCoords.vy + 0xfa0000;
	line_info.EndCoords.vz = line_info.StartCoords.vz;
	M3dColij_InitLineInfo(&line_info);
	M3dZone_LineToItem(&line_info, 1);

	if (line_info.pItem && line_info.Normal.vy < -1900 && mPhysicsState != 4 &&
	    mPhysicsState != 8) {
		{
			int _x = line_info.Position.vx;
			int _y = line_info.Position.vy;
			int _z = line_info.Position.vz;

			mShadowPos.vx = _x;
			mShadowPos.vy = _y;
			mShadowPos.vz = _z;
		}

		{
			int _x = line_info.Normal.vx;
			int _z = line_info.Normal.vz;
			int _y = line_info.Normal.vy;

			mShadowNormal.vx = _x;
			mShadowNormal.vy = _y;
			mShadowNormal.vz = _z;
		}

		if (GNumberOfPlayers == 1) {
			mShadowLights = 1;
			mpShadowRGB->r = mpShadowRGB->g = mpShadowRGB->b =
			    ((4300 - line_info.Distance) << 7) / 4000;
		}
	} else {
		mShadowLights = 0;
	}
}

/* InitItem__6CBrucePc (0x80042988) */
/* CItem::InitItem, then reads this skater's hook points packet. */
void CBruce::InitItem(char *pName) {
	CItem::InitItem(pName);
	M3dUtils_ReadHooksPacket(this, &SkaterHooks[mSkaterIndex]);
}

/* FlipToVelocity__6CBruce (0x800429d0) */
/* C++ (PHYSICS.CPP-style CBruce method): flips the skater about the up axis by negating the
   front and right columns of the rotation matrix. The function finder split it in three
   (0x800429d0, 0x80042a54, 0x80042aa0). */
void CBruce::FlipToVelocity() {
	if (mAnim == 34)
		return;

	if (Mips_CVecDotProd(&mFront, &mVel) <= 20480)
		return;

	mRotMatrix.m[0][2] = -mFront.vx;
	mFront.vx = mRotMatrix.m[0][2];
	mRotMatrix.m[1][2] = -mFront.vy;
	mFront.vy = mRotMatrix.m[1][2];
	mRotMatrix.m[2][2] = -mFront.vz;
	mFront.vz = mRotMatrix.m[2][2];
	mRotMatrix.m[0][0] = -mRight.vx;
	mRight.vx = mRotMatrix.m[0][0];
	mRotMatrix.m[1][0] = -mRight.vy;
	mRight.vy = mRotMatrix.m[1][0];
	mRotMatrix.m[2][0] = -mRight.vz;
	mRight.vz = mRotMatrix.m[2][0];
	mExtraFlags ^= 2;
}

/* BloodSplat__6CBruceii (0x80042ab0) */
void CBruce::BloodSplat(int BodyPart, int MaxBloodBits) {
	if (Cheat_Blood) {
		Video_BloodSplat(this, BodyPart, MaxBloodBits);

		if (BitCount < 200) {
			CSVector v;
			CVector Pos;
			CSVector Cone;
			int Bits;
			int FloorY;
			SLineInfo BloodLine;

			v.vx = -1024;
			v.vy = 0;
			v.vz = 0;
			Bit_SetSparkTrajectory(v);

			if (BodyPart == 0) {
				Cone.vx = 1200;
				Cone.vy = 800;
				Cone.vz = 0;
			} else if (BodyPart == 1) {
				Cone.vx = 1600;
				Cone.vy = 1600;
				Cone.vz = 0;
			} else {
				Cone.vx = 1200;
				Cone.vy = 800;
				Cone.vz = 0;
			}

			Bit_SetSparkTrajectoryCone(Cone);
			Bit_SetSparkRGB(72, 0, 0);
			Bit_SetSparkFadeRGB(0, 0, 0);
			GetHookPosition(Pos, BodyPart);

			SparkSemiTrans = 0;
			UseCacheForBits = 0;
			Bits = 200 - BitCount;

			if (MaxBloodBits < Bits)
				Bits = MaxBloodBits;

			BloodLine.StartCoords.vx = Pos.vx;
			BloodLine.StartCoords.vy = Pos.vy - 163840;
			BloodLine.StartCoords.vz = Pos.vz;
			BloodLine.EndCoords.vx = Pos.vx;
			BloodLine.EndCoords.vy = Pos.vy + 3276800;
			BloodLine.EndCoords.vz = Pos.vz;
			M3dColij_InitLineInfo(&BloodLine);
			M3dZone_LineToItem(&BloodLine, 1);

			if (BloodLine.pItem) {
				FloorY = BloodLine.Position.vy - 16384;
			} else {
				FloorY = Pos.vy - 16384;
			}

			Pos.vy -= 122880;

			{
				int i;

				for (i = 0; i < Bits; i++) {
					CSpark *pSpark;

					if (Mem_GetFree(1) < 10000)
						return;

					Bit_SetSparkSize((i & 1) + 1);

					pSpark = new CSpark(Pos, (i >> 2) + 14, 10240 + i * 256, (i & 7) + 50);

					pSpark->mVel += mVel;
					pSpark->mFloorY = FloorY;
				}
			}

			UseCacheForBits = 1;
			SparkSemiTrans = 1;
		}
	}
}

/* AI__6CBruce (0x80042d04) */
/* : CBruce::AI (vtable slot 2). The fields at +0x890, +0x8fc/+0x8fd and +0x91c have
   no names in bruce.h yet: reached through its padding arrays. */
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

			if (this == MechList)
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

				if ((Cheat_FPS || Cheat_StatsOverride) && this == GSkater[0]) {
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

/* HandleWater__6CBruce (0x80043258) */
void CBruce::HandleWater() {
	int WaterLevel;
	int bottom;

	if (GNumberOfPlayers != 2) {
		if (this == MechList)
			WaterLevel = CurrentWaterLevel;
		else
			WaterLevel = CurrentWaterLevel2;

		bottom = mPos.vy + 0x19000;

		if (WaterLevel < bottom) {
			if (!(mFlags & 8))
				SFX_PlayX(SND_SPLASH1, 8192, 0);

			mFlags |= 8;

			if (!(TTime & 3)) {
				CVector p;
				int offset;

				p = mPos;
				p.vy = WaterLevel;
				offset = Rnd(24) + 16;

				{
					SSinCos *b;
					int idx = Rnd(4096) & 4095;

					b = rcossin_tbl;
					SSinCos *e = b + idx;

					p.vx += offset * e->s;
					p.vz += offset * e->c;
				}

				new CFlatRipple(p, Rnd(24) + 40, 221, 238, 255, 12, Rnd(3) + 4);
			}
		} else {
			mFlags &= ~8;
		}
	}
}

/* SFX_Grunt__FP6CBruce (0x80043408) */
void SFX_Grunt(CBruce *p) {
	int a;

	switch (Face_Terrain) {
		case 7:
			a = SND_FENCEHIT2;
			break;
		case 9:
			a = SND_SMACKMETAL;

			if (GLevel == 8)
				a = SND_SPARK8;

			break;
		default:
			goto out;
	}

	SFX_PlayX((ESfx)a, 8192, 0);
out:
	a = Stats[p->mSkaterIndex].ComboID;

	if (a == 0x9000 || a == 0xb000)
		a = SND_GRUNTGIRL;
	else
		a = SND_GRUNT + Rnd(3);

	SFX_PlayX((ESfx)a, 8192, 0);
}
