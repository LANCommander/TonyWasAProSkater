/* PutDataChunk__FP6CBruce (0x80069974) */
/* C++ (VIDEO.CPP). Locals, their order and types come from the demo's MAIN.SYM. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/video.h"
#include "game/bruce.h"

enum ESoundCmdType { SOUNDCMD_NONE, SOUNDCMD_PLAY, SOUNDCMD_STOP };

extern SVideoInfo GVInfo1, GVInfo2;
extern int PosBits[], EulerBits[], FlipAngleBits[], PrefixCode[], PrefixBits[];
extern CBruce *MechList;
extern volatile int Xblanks;

int Splat1, Splat2, TrickOb1, TrickOb2;

int BitsRequired(long);
void PutNBits(SVideoInfo *, long, int);
void Video_Stop(CBruce *);
void Matrix2Euler(MATRIX *, SVECTOR *);
void DequeueSoundCmd(ESoundCmdType &, int &);
extern "C" int HasSoundCmd__Fv(void);

void PutDataChunk(CBruce *pSkater) {
	int i, bx, by, bz;
	int PredX, PredY, PredZ;
	int DeltaX, DeltaY, DeltaZ;
	CVector PredPos, DeltaPos;
	SVideoInfo *VInfo;

	if (pSkater == MechList)
		VInfo = &GVInfo1;
	else
		VInfo = &GVInfo2;

	if (VInfo->pStream + 6 >= VInfo->pStreamStart + 6000) {
		Video_Stop(pSkater);
		return;
	}

	VInfo->NumFrames++;

	PredX = VInfo->OldPos.vx * 2 - VInfo->OldPos2.vx;
	PredY = VInfo->OldPos.vy * 2 - VInfo->OldPos2.vy;
	PredZ = VInfo->OldPos.vz * 2 - VInfo->OldPos2.vz;
	DeltaX = (pSkater->mPos.vx >> 12) - PredX;
	DeltaY = (pSkater->mPos.vy >> 12) - PredY;
	DeltaZ = (pSkater->mPos.vz >> 12) - PredZ;
	VInfo->OldPos2 = VInfo->OldPos;
	VInfo->OldPos = pSkater->mPos >> 12;
	bx = BitsRequired(DeltaX);
	by = BitsRequired(DeltaY);
	bz = BitsRequired(DeltaZ);

	if (bx < by)
		bx = by;

	if (bx < bz)
		bx = bz;

	for (i = 0; i < 6; i++)
		if (PosBits[i] >= bx)
			break;

	PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
	PutNBits(VInfo, DeltaX, PosBits[i]);
	PutNBits(VInfo, DeltaY, PosBits[i]);
	PutNBits(VInfo, DeltaZ, PosBits[i]);

	Matrix2Euler(&pSkater->mRotMatrix, &VInfo->Euler);

	DeltaX = ((VInfo->Euler.vx & 0xfff) - VInfo->OldEuler.vx + 0x4000) & 0xfff;
	DeltaY = ((VInfo->Euler.vy & 0xfff) - VInfo->OldEuler.vy + 0x4000) & 0xfff;
	DeltaZ = ((VInfo->Euler.vz & 0xfff) - VInfo->OldEuler.vz + 0x4000) & 0xfff;
	VInfo->Euler.vx &= 0xfff;
	VInfo->Euler.vy &= 0xfff;
	VInfo->Euler.vz &= 0xfff;

	if (DeltaX >= 2048)
		DeltaX -= 4096;

	if (DeltaY >= 2048)
		DeltaY -= 4096;

	if (DeltaZ >= 2048)
		DeltaZ -= 4096;

	VInfo->OldEuler = VInfo->Euler;
	bx = BitsRequired(DeltaX);
	by = BitsRequired(DeltaY);
	bz = BitsRequired(DeltaZ);

	if (bx < by)
		bx = by;

	if (bx < bz)
		bx = bz;

	for (i = 0; i < 6; i++)
		if (EulerBits[i] >= bx)
			break;

	PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
	PutNBits(VInfo, DeltaX, EulerBits[i]);
	PutNBits(VInfo, DeltaY, EulerBits[i]);
	PutNBits(VInfo, DeltaZ, EulerBits[i]);

	if (pSkater->mPhysicsState == VInfo->OldPhysicsState)
		PutNBits(VInfo, 0, 1);
	else
		PutNBits(VInfo, (pSkater->mPhysicsState << 1) | 1, 5);

	VInfo->OldPhysicsState = pSkater->mPhysicsState;

	if (pSkater->mAnim == VInfo->OldAnim)
		PutNBits(VInfo, 0, 1);
	else
		PutNBits(VInfo, (pSkater->mAnim << 1) | 1, 8);

	VInfo->OldAnim = pSkater->mAnim;
	DeltaX = pSkater->mFrame - VInfo->OldFrame;

	if ((unsigned)DeltaX < 4)
		PutNBits(VInfo, DeltaX << 1, 3);
	else
		PutNBits(VInfo, (pSkater->mFrame << 1) | 1, 9);

	VInfo->OldFrame = pSkater->mFrame;
	PutNBits(VInfo, (pSkater->mExtraFlags >> 1) & 1, 1);

	if (!pSkater->m720) {
		PutNBits(VInfo, 0, 1);

		VInfo->OldFlipAngle = 0;
		VInfo->FlipAngleDelta = 0;
	} else {
		PutNBits(VInfo, 1, 1);

		{
			int Delta, Bits;

			Delta = pSkater->m724 - (VInfo->OldFlipAngle + VInfo->FlipAngleDelta);
			Bits = 0;

			if (Delta)
				Bits = BitsRequired(Delta);

			for (i = 0; i < 6; i++)
				if (FlipAngleBits[i] >= Bits)
					break;

			PutNBits(VInfo, PrefixCode[i], PrefixBits[i]);
			PutNBits(VInfo, Delta, FlipAngleBits[i]);

			VInfo->OldFlipAngle = pSkater->m724;
			VInfo->FlipAngleDelta = Delta;
		}
	}

	DeltaX = Xblanks - VInfo->OldXblanks;

	if (DeltaX >= 6) {
		VInfo->OldXblanks = Xblanks - (DeltaX - 5);
		DeltaX = 5;
	} else
		VInfo->OldXblanks = Xblanks;

	PutNBits(VInfo, DeltaX - 2, 2);

	if (HasSoundCmd__Fv()) {
		PutNBits(VInfo, 1, 1);

		{
			ESoundCmdType SoundCmdType;
			int SoundNum;

			DequeueSoundCmd(SoundCmdType, SoundNum);

			if (SoundCmdType == SOUNDCMD_PLAY) {
				PutNBits(VInfo, 1, 1);
				PutNBits(VInfo, SoundNum, 8);
			} else if (SoundCmdType == SOUNDCMD_STOP) {
				PutNBits(VInfo, 0, 1);
				PutNBits(VInfo, SoundNum, 8);
			}
		}
	} else
		PutNBits(VInfo, 0, 1);

	if (Splat1 == -1 && Splat2 == Splat1 && TrickOb1 == Splat2 &&
	    TrickOb2 == TrickOb1) {
		PutNBits(VInfo, 0, 1);
		return;
	}

	PutNBits(VInfo, 1, 1);

	if (pSkater == MechList) {
		if (Splat1 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, Splat1, 2);

			Splat1 = -1;
		} else
			PutNBits(VInfo, 0, 1);
	} else {
		if (Splat2 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, Splat2, 2);

			Splat2 = -1;
		} else
			PutNBits(VInfo, 0, 1);
	}

	if (pSkater == MechList) {
		if (TrickOb1 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, TrickOb1, 8);

			TrickOb1 = -1;
			return;
		}
	} else {
		if (TrickOb2 != -1) {
			PutNBits(VInfo, 1, 1);
			PutNBits(VInfo, TrickOb2, 8);

			TrickOb2 = -1;
			return;
		}
	}

	PutNBits(VInfo, 0, 1);
}
