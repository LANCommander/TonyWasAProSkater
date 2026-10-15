/* CollideWithSkater__FP4CCarP6CBruceRC6MATRIXPs (0x8006b6c0) */
/* C++ (CAR.CPP). Fuzzy demo twin (0.96); locals from its MAIN.SYM. Tests the skater's last move
   against a car's bounding box, in car space via the GTE. A grounded skater landing on top bounces
   off with sparks. Otherwise the skater is knocked down, thrown sideways or backwards, and the car
   honks (by type). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <stdlib.h>
}

#define ABS(x)     (((x) ^ ((x) >> 31)) - ((x) >> 31))

#include "game/car.h"
#include "game/bruce.h"
#include "game/bit.h"
#include "game/sfx.h"
#include "game/front.h"

extern LocalText GameText;
extern int BitCount;
extern int UseCacheForBits;
extern char ZeroVector[];

unsigned int Utils_CrapDist(const CVector &, const CVector &);
int Redbook_Rnd(int);
void Bit_SetSparkTrajectory(const CSVector &);

void CollideWithSkater(CCar *pCar, CBruce *pSkater, const MATRIX &tm, short *pBox) {
	if (pCar == pSkater->mLastCar)
		return;

	if (pSkater->mInAir && pSkater->mPhysicsState)
		return;

	if (Utils_CrapDist(pSkater->mPos, pCar->mPos) >= 500)
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

			if (pCar->mType == 215)
				CarHeight = 0x46000;
			else if (pCar->mType == 216)
				CarHeight = 0x64000;

			if (!pSkater->mInAir && pSkater->mPhysicsState == 1 &&
			    CarHeight < pCar->mPos.vy - pSkater->mPos.vy) {
				pSkater->mVel.vx >>= 1;
				pSkater->mVel.vy >>= 1;
				pSkater->mVel.vz >>= 1;
				pSkater->mVel.vy -= 0x1e000;
				pSkater->Vibrate(8, 1, 150);
				SFX_Grunt(pSkater);
				SFX_PlayX(40, 0x2000, 0);
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
						new CGrindSpark(pSkater, *(CVector *)ZeroVector, -40, 4096, 5);
					}

					UseCacheForBits = 1;
				}

				return;
			}
		}

		switch (Redbook_Rnd(16) >> 3) {
			case 0: {
				int vx = pCar->mVel.vx;
				int vz = pCar->mVel.vz;

				if (ABS(vx) > ABS(vz)) {
					int t;

					vx += vx >> 1;
					t = Redbook_Rnd(vx >> 1) + (vx >> 2);

					if (Redbook_Rnd(16) >> 3)
						t = -t;

					vz += t;
				} else {
					int t;

					vz += vz >> 1;
					t = Redbook_Rnd(vz >> 1) + (vz >> 2);

					if (Redbook_Rnd(16) >> 3)
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
		SFX_PlayX(114, 0x2000, 0);

		{
			int sfx;

			switch (pCar->mType) {
				case 213:
					sfx = Redbook_Rnd(4) + 115;
					break;
				case 214:
					sfx = Redbook_Rnd(4) + 125;
					break;
				case 215:
					sfx = 131;
					break;
				case 216:
					sfx = 120;
					break;
				default:
					return;
			}

			SFX_PlayX(sfx, 0x2000, 0);
		}
	}
}
