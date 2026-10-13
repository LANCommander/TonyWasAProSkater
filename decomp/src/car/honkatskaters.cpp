/* HonkAtSkaters__FP4CCarG7CVector (0x8006bd44) */
/* C++ (CAR.CPP). Demo twin HonkAtSkaters (0.26): THPS1 has no skater table, so it checks the
   player skater (MechList) and the one linked after it. A car honks (by type) once when a
   skater within 2000 units is inside the cone ahead of it; the flag at +0x220 rearms when clear.
   Mips_DotProd2Sign = 64-bit cross-product sign helper, SFX_PlayX__FUisi = SFX_PlayX. Each honk case
   makes its own SFX call; cross-jumping merges them into retail's single jal. */
#include "game/car.h"
#include "game/bruce.h"
#include "game/video.h"
#include "game/sfx.h"

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern CBruce *MechList;

int Utils_CrapDist(const CVector &, const CVector &);
int Redbook_Rnd(int);

extern "C" {
	int Mips_DotProd2Sign(int, int, int, int);
}

void HonkAtSkaters(CCar *pCar, CVector Front) {
	int Honk = 0;

	if (GVInfo1.VideoMode == 2 || GVInfo2.VideoMode == 2)
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
				case 213:
					SFX_PlayX(Redbook_Rnd(3) + 110, 0x2000, 0);
					break;
				case 214:
					SFX_PlayX(122, 0x2000, 0);
					break;
				case 215:
					SFX_PlayX(130, 0x2000, 0);
					break;
				case 216:
					SFX_PlayX(120, 0x2000, 0);
					break;
			}

			pCar->mHonkLock = 1;
		}
	} else
		pCar->mHonkLock = 0;
}
