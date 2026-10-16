/* CAR.CPP: road traffic, CCar, a baddy following road segments and honking at skaters
   (declarations in game/car.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <stdlib.h>
}

#include "game/car.h"
#include "game/bruce.h"
#include "game/bit.h"
#include "game/sfx.h"
#include "game/script.h"
#include "game/front.h"
#include "game/m3d.h"
#include "game/video.h"
#include "game/bit2.h"
#include "game/globals.h"
#include "game/m3dutils.h"
#include "game/physics.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/mech.h"

extern int CarVol;
extern "C" char __vt_4CCar[];
extern "C" int BASEF(CCar *p, int a);
extern "C" void __dl__5CItemPv(void *p);
#define ABS(x)     (((x) ^ ((x) >> 31)) - ((x) >> 31))
void CollideWithSkater(CCar *, CBruce *, const MATRIX &, short *);
extern "C" {
	int Mips_DotProd2Sign(int, int, int, int);
}
int GetDeltaAngle(int, int);
void CollideWithSkaters(CCar *);
void HonkAtSkaters(CCar *, CVector);

/* __4CCarPsii (0x8006b490) */
/* A car (taxi, police car, van or cable car by `type`): its model and engine loop,
   on the baddy list, cycling animation 0, placed from its object data at node `node` and set on
   the road segment that node links to. */
CCar::CCar(short *p_info, int node, int type) {
	ushort segment;

	mType = type;
	mLoopingSoundUpdate = 1;

	switch (type) {
		case OBJ_CAR_TAXI:
			InitItem("c_taxi");

			mLoopingSound = SFX_PlayX(SFX_TAXI_ENGINE, 0, 0);
			break;
		case OBJ_CAR_POLICE:
			mLoopingSoundUpdate = 0;
			InitItem("c_police");
			break;
		case OBJ_CAR_BUS:
			InitItem("c_van");

			mLoopingSound = SFX_PlayX(SFX_VAN_ENGINE, 0, 0);
			break;
		case OBJ_CAR_CABLE:
			InitItem("c_cable");

			mLoopingSound = SFX_PlayX(SFX_CABLE_CAR_LOOP, 0, 0);
			break;
	}

	CarVol = 300;
	AttachTo(&BaddyList);

	mAnimSpeed = 0x10000;
	CycleAnim(0, 1);

	mGroundToOrigin = 52;
	mOriginToEye = 32;
	mHealth = 60;
	SquirtAngles(SquirtPos(p_info));

	mNode = node;
	segment = Trig_GetLinksPointer(node)[1];
	mCurrentRoadSegment = segment;
	Trig_GetPosition(mPos, segment);

	mPos.vy -= 0x6e000;
}

/* FUN_8006b630 (0x8006b630) */
/* CCar's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
extern "C" void FUN_8006b630(CCar *p, int f) {
	p->__vptr = __vt_4CCar;
	p->DeleteFrom(&BaddyList);
	BASEF(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

/* GetDeltaAngle__Fii (0x8006b694) */
int GetDeltaAngle(int a, int b) {
	b -= a;

	if (b < -2048)
		b += 4096;

	if (b >= 2048)
		b -= 4096;

	return b;
}

/* CollideWithSkater__FP4CCarP6CBruceRC6MATRIXPs (0x8006b6c0) */
/* Fuzzy demo twin (0.96); locals from its MAIN.SYM. Tests the skater's last move
   against a car's bounding box, in car space via the GTE. A grounded skater landing on top bounces
   off with sparks. Otherwise the skater is knocked down, thrown sideways or backwards, and the car
   honks (by type). */
void CollideWithSkater(CCar *pCar, CBruce *pSkater, const MATRIX &tm, short *pBox) {
	if (pCar == pSkater->mLastCar)
		return;

	if (pSkater->mInAir && pSkater->mPhysicsState)
		return;

	if ((unsigned int)Utils_CrapDist(pSkater->mPos, pCar->mPos) >= 500)
		return;

	{
		VECTOR OldPos;
		VECTOR Pos;
		int OldX, OldY, OldZ;
		int NewX, NewY, NewZ;
		int XMin, XMax, YMin, YMax, ZMin, ZMax;
		int tmp;

		OldX = pSkater->mOldPos.vx - pCar->mPos.vx;
		OldY = pSkater->mOldPos.vy - pCar->mPos.vy;
		OldZ = pSkater->mOldPos.vz - pCar->mPos.vz;
		NewX = pSkater->mPos.vx - pCar->mPos.vx;
		NewY = pSkater->mPos.vy - pCar->mPos.vy;
		NewZ = pSkater->mPos.vz - pCar->mPos.vz;
		{
			/* into car space, unshifted (16 bits per coordinate in) */
			GeomAcc r;

			geom_ld_mat(pc_geom.rot, &tm);
			r = geom_mvmva(pc_geom.rot, 0, (int16_t)(OldX >> 12), (int16_t)(OldY >> 12),
			               (int16_t)(OldZ >> 12), 0, 0, 0);
			geom_st_lv(&OldPos, r.mac);
			r = geom_mvmva(pc_geom.rot, 0, (int16_t)(NewX >> 12), (int16_t)(NewY >> 12),
			               (int16_t)(NewZ >> 12), 0, 0, 0);
			geom_st_lv(&Pos, r.mac);
		}

		XMin = OldPos.vx;
		XMax = Pos.vx;

		if (XMax < XMin) {
			tmp = XMin;
			XMin = XMax;
			XMax = tmp;
		}

		XMin -= 0x14000;
		XMax += 0x14000;
		YMin = OldPos.vy;
		YMax = Pos.vy;

		if (YMax < YMin) {
			tmp = YMin;
			YMin = YMax;
			YMax = tmp;
		}

		YMin -= 0x14000;
		YMax += 0x14000;
		ZMin = OldPos.vz;
		ZMax = Pos.vz;

		if (ZMax < ZMin) {
			tmp = ZMin;
			ZMin = ZMax;
			ZMax = tmp;
		}

		ZMin -= 0x14000;
		ZMax += 0x14000;

		if (XMax < pBox[1] << 12 || pBox[0] << 12 < XMin)
			return;

		if (ZMax < pBox[5] << 12 || pBox[4] << 12 < ZMin)
			return;

		if (YMax < pBox[3] << 12 || pBox[2] << 12 < YMin)
			return;

		{
			int CarHeight = 0;

			if (pCar->mType == OBJ_CAR_BUS)
				CarHeight = 0x46000;
			else if (pCar->mType == OBJ_CAR_CABLE)
				CarHeight = 0x64000;

			if (!pSkater->mInAir && pSkater->mPhysicsState == 1 &&
			    CarHeight < pCar->mPos.vy - pSkater->mPos.vy) {
				pSkater->mVel.vx >>= 1;
				pSkater->mVel.vy >>= 1;
				pSkater->mVel.vz >>= 1;
				pSkater->mVel.vy -= 0x1e000;
				pSkater->Vibrate(8, 1, 150);
				SFX_Grunt(pSkater);
				SFX_PlayX(SFX_CLANG, 0x2000, 0);
				pSkater->Trick_Trick(78, GameText.pCarPlant, -1000, 0);

				{
					int n = BitCount;

					pSkater->mLastCar = pCar;

					if (n >= 200)
						return;
				}

				pSkater->SetAnglesFromHeading();
				Bit_SetSparkTrajectory(pSkater->mAngles);

				{
					CSVector cone;
					int i;

					cone.vx = 450;
					cone.vy = 900;
					cone.vz = 0;
					Bit_SetSparkTrajectoryCone(cone);

					UseCacheForBits = 0;

					for (i = 0; i < 20; i++) {
						new CGrindSpark(pSkater, ZeroVector, -40, 4096, 5);
					}

					UseCacheForBits = 1;
				}

				return;
			}
		}

		switch (Rnd(16) >> 3) {
			case 0: {
				int vx = pCar->mVel.vx;
				int vz = pCar->mVel.vz;

				if (ABS(vx) > ABS(vz)) {
					int t;

					vx += vx >> 1;
					t = Rnd(vx >> 1) + (vx >> 2);

					if (Rnd(16) >> 3)
						t = -t;

					vz += t;
				} else {
					int t;

					vz += vz >> 1;
					t = Rnd(vz >> 1) + (vz >> 2);

					if (Rnd(16) >> 3)
						t = -t;

					vx += t;
				}

				pSkater->mVel.vx = vx;
				pSkater->mVel.vy = pCar->mVel.vy - 0x16800;
				pSkater->mVel.vz = vz;
				break;
			}
			case 1:
				pSkater->mVel.vx = -pCar->mVel.vx >> 2;
				pSkater->mVel.vy = -0x2d000;
				pSkater->mVel.vz = -pCar->mVel.vz >> 2;
				break;
		}

		pSkater->mLastCar = pCar;
		pSkater->Bail(1);
		pSkater->BloodSplat(1, 100);
		pSkater->Vibrate(12, 1, 255);
		SFX_Grunt(pSkater);
		SFX_PlayX(SFX_CAR_HIT, 0x2000, 0);

		{
			ESfx sfx;

			switch (pCar->mType) {
				case OBJ_CAR_TAXI:
					sfx = (ESfx)(SFX_TAXI_HIT_1 + Rnd(4));
					break;
				case OBJ_CAR_POLICE:
					sfx = (ESfx)(SFX_POLICE_HIT_1 + Rnd(4));
					break;
				case OBJ_CAR_BUS:
					sfx = SFX_VAN_HIT;
					break;
				case OBJ_CAR_CABLE:
					sfx = SFX_CABLE_CAR_BELL;
					break;
				default:
					return;
			}

			SFX_PlayX(sfx, 0x2000, 0);
		}
	}
}

/* CollideWithSkaters__FP4CCar (0x8006bc88) */
/* Builds the object's rotation matrix and runs CollideWithSkater against the skater and its partner
   (mNextItem), unless a replay/video mode 2 is active on either player. */
void CollideWithSkaters(CCar *p) {
	MATRIX m;
	SModel **info;
	CBruce *s1, *s2;
	short *x;

	if (GVInfo1[0].VideoMode == 2 || GVInfo1[1].VideoMode == 2)
		return;

	info = PSXRegion[p->mRegion].ppModels;
	s1 = MechList;
	s2 = (CBruce *)s1->mNextItem;
	/* model 4's bounding box (file data) */
	x = (short *)((char *)info[4] + 12);
	RotMatrix((SVECTOR *)&p->mTransform, &m);
	CollideWithSkater(p, s1, m, x);

	if (s2)
		CollideWithSkater(p, s2, m, x);
}

/* HonkAtSkaters__FP4CCarG7CVector (0x8006bd44) */
/* Demo twin HonkAtSkaters (0.26): THPS1 has no skater table, so it checks the
   player skater (MechList) and the one linked after it. A car honks (by type) once when a
   skater within 2000 units is inside the cone ahead of it; the flag at +0x220 rearms when clear.
   Mips_DotProd2Sign = 64-bit cross-product sign helper, SFX_PlayX__FUisi = SFX_PlayX. Each honk case
   makes its own SFX call; cross-jumping merges them into retail's single jal. */
void HonkAtSkaters(CCar *pCar, CVector Front) {
	int Honk = 0;

	if (GVInfo1[0].VideoMode == 2 || GVInfo1[1].VideoMode == 2)
		return;

	CBruce *pSkater = MechList;
	CBruce *pOther = (CBruce *)pSkater->mNextItem;

	if (Utils_CrapDist(pCar->mPos, pSkater->mPos) < 2000) {
		int LeftX = -Front.vz + Front.vx;
		int LeftZ = Front.vx + Front.vz;
		int VecX = pSkater->mPos.vx - pCar->mPos.vx;
		int VecZ = pSkater->mPos.vz - pCar->mPos.vz;

		if (Mips_DotProd2Sign(LeftX, VecX, LeftZ, VecZ) > 0) {
			int RightX = Front.vz + Front.vx;
			int RightZ = Front.vz - Front.vx;

			Honk = Mips_DotProd2Sign(RightX, VecX, RightZ, VecZ) > 0;
		}
	}

	if (pOther && !Honk) {
		if (Utils_CrapDist(pCar->mPos, pOther->mPos) < 2000) {
			int LeftX = -Front.vz + Front.vx;
			int LeftZ = Front.vx + Front.vz;
			int VecX = pOther->mPos.vx - pCar->mPos.vx;
			int VecZ = pOther->mPos.vz - pCar->mPos.vz;

			if (Mips_DotProd2Sign(LeftX, VecX, LeftZ, VecZ) > 0) {
				int RightX = Front.vz + Front.vx;
				int RightZ = Front.vz - Front.vx;

				if (Mips_DotProd2Sign(RightX, VecX, RightZ, VecZ) > 0)
					Honk = 1;
			}
		}
	}

	if (Honk) {
		if (!pCar->mHonkLock) {
			switch (pCar->mType) {
				case OBJ_CAR_TAXI:
					SFX_PlayX((ESfx)(SFX_TAXI_HORN_1 + Rnd(3)), 0x2000, 0);
					break;
				case OBJ_CAR_POLICE:
					SFX_PlayX(SFX_POLICE_HORN, 0x2000, 0);
					break;
				case OBJ_CAR_BUS:
					SFX_PlayX(SFX_VAN_HORN, 0x2000, 0);
					break;
				case OBJ_CAR_CABLE:
					SFX_PlayX(SFX_CABLE_CAR_BELL, 0x2000, 0);
					break;
			}

			pCar->mHonkLock = 1;
		}
	} else
		pCar->mHonkLock = 0;
}

/* AI__4CCar (0x8006bf5c) */
/* Locals, their order and types come from the demo's MAIN.SYM, statements from its
   line table; field offsets are THPS1's. */
void CCar::AI(void) {
	CVector SegStart, SegEnd, SegDir;
	int Speed, InitialSpeed, RemainingSpeed, NextSegment, Length, t, MajorAxis;
	int FirstTimeThroughLoop;
	int TargetAngle, d;

	unsigned int snd;

	if (mLoopingSoundUpdate != -1 && (snd = mLoopingSound)) {
		int m = XFrames % mLoopingSoundUpdate;
		int t = SFXLEVEL;

		SFXLEVEL = t * CarVol / 100;

		if (!m)
			SFX_ModifyPos(snd, mPos, 0);

		SFXLEVEL = t;
	}

	FirstTimeThroughLoop = 1;
	M3d_BuildTransform((CSuper *)this);
	CollideWithSkaters(this);

	do {
		NextSegment = Trig_GetLinksPointer(mCurrentRoadSegment)[1];
		Trig_GetPosition(SegStart, mCurrentRoadSegment);

		SegStart.vy += -0x6e000;
		Trig_GetPosition(SegEnd, NextSegment);

		SegEnd.vy += -0x6e000;
		SegDir = SegEnd - SegStart;
		M3dUtils_NormaliseVector(&SegDir);

		if (FirstTimeThroughLoop) {
			FirstTimeThroughLoop = 0;
			Speed = Mips_CVecDotProd(&SegDir, &mVel);
			InitialSpeed = Speed;

			if (Speed < 0)
				Speed = 0;

			if (Speed < 0x50000)
				Speed += 4096;

			mVel.vx = Mips_FixMul(SegDir.vx, Speed);
			mVel.vy = Mips_FixMul(SegDir.vy, Speed);
			mVel.vz = Mips_FixMul(SegDir.vz, Speed);
		} else {
			mVel.vx = Mips_FixMul(SegDir.vx, InitialSpeed);
			mVel.vy = Mips_FixMul(SegDir.vy, InitialSpeed);
			mVel.vz = Mips_FixMul(SegDir.vz, InitialSpeed);
		}

		MajorAxis = 0;

		if (ABS(SegDir.vy) > ABS(SegDir.vx)) {
			MajorAxis = 1;

			if (ABS(SegDir.vy) < ABS(SegDir.vz))
				MajorAxis = 2;
		} else if (ABS(SegDir.vx) < ABS(SegDir.vz)) {
			MajorAxis = 2;
		}

		switch (MajorAxis) {
			case 0:
				Length = ((SegEnd.vx - SegStart.vx) / SegDir.vx) << 12;
				t = ((mPos.vx - SegStart.vx) / SegDir.vx) << 12;
				break;
			case 1:
				Length = ((SegEnd.vy - SegStart.vy) / SegDir.vy) << 12;
				t = ((mPos.vy - SegStart.vy) / SegDir.vy) << 12;
				break;
			case 2:
				Length = ((SegEnd.vz - SegStart.vz) / SegDir.vz) << 12;
				t = ((mPos.vz - SegStart.vz) / SegDir.vz) << 12;
				break;
		}

		if (Length - (Length >> 4) < t) {
			Speed -= Speed >> 4;
			mVel.vx -= mVel.vx >> 4;
			mVel.vy -= mVel.vy >> 4;
			mVel.vz -= mVel.vz >> 4;
		}

		t += Speed;

		if (t >= 0 && t <= Length) {
			mPos.vx += Mips_FixMul(SegDir.vx, Speed);
			mPos.vy += Mips_FixMul(SegDir.vy, Speed);
			mPos.vz += Mips_FixMul(SegDir.vz, Speed);
			RemainingSpeed = 0;
		} else {
			Trig_GetLinksPointer(NextSegment);

			mCurrentRoadSegment = NextSegment;
			RemainingSpeed = t - Length;
			mPos = SegEnd;
		}

		Speed = RemainingSpeed;
	} while (RemainingSpeed);

	TargetAngle = ratan2(-SegDir.vx, -SegDir.vz) & 4095;
	d = GetDeltaAngle(mAngles.vy, TargetAngle);

	if (ABS(d) < 128)
		mAngles.vy = TargetAngle;
	else
		mAngles.vy = (mAngles.vy + (d >> 2)) & 4095;

	TargetAngle = ratan2(SegDir.vy, 4096) & 4095;
	d = GetDeltaAngle(mAngles.vx, TargetAngle);

	if (ABS(d) < 128)
		mAngles.vx = TargetAngle;
	else
		mAngles.vx = (mAngles.vx + (d >> 3)) & 4095;

	HonkAtSkaters(this, SegDir);
}

/* FUN_8006c470 (0x8006c470) */
int FUN_8006c470(void) {
	return 1;
}
