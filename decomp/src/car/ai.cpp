/* AI__4CCar (0x8006bf5c) */
/* C++ (CAR.CPP). Locals, their order and types come from the demo's MAIN.SYM, statements from its
   line table; field offsets are THPS1's. */
#include "game/car.h"

extern int XFrames;
extern int SFXLEVEL;

int CarVol;

unsigned short *Trig_GetLinksPointer(int);
void *Trig_GetPosition(CVector &, int); /* returns the node data past the position */
void SFX_ModifyPos(unsigned int, const CVector &, int);
int GetDeltaAngle(int, int);
void CollideWithSkaters(CCar *);
void HonkAtSkaters(CCar *, CVector);
int Mips_FixMul(int, int);
int Mips_CVecDotProd(CVector *, CVector *);
void M3dUtils_NormaliseVector(CVector *);
void M3d_BuildTransform(CSuper *);

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

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
