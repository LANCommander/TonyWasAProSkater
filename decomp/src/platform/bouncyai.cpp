/* BouncyAI__9CPlatform (0x8003db00) */
/* C++ (PLATFORM.CPP). Fuzzy demo twin (opcode-identical but for a few words); locals and blocks from its
   MAIN.SYM. THPS1's CPlatform fields sit 8 bytes past the demo's; the CBaddy part is laid out by offset. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/platform.h"
#include "game/colij.h"

extern "C" void FUN_8008b3d4(CSVector *, MATRIX *);
void M3dColij_InitLineInfo(SLineInfo *);
void M3dZone_LineToItemNotSCached(SLineInfo *, int);
int Mips_CVecDotProd(CVector *, CVector *);
int Mips_FixMul(int, int);
int Rnd(int);
int SFX_PlayX(unsigned int, short, int);

extern int GLevel;

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

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
	FUN_8008b3d4(&mAngles, &m);

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
				FUN_8008b3d4(&mAngles, &m);

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
