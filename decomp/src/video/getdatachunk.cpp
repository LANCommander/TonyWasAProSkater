/* GetDataChunk__FP6CBrucei (0x8006a158) flags=-fstructure-noalias */
/* C++ (VIDEO.CPP). Locals, their order and types come from the demo's MAIN.SYM. */
extern "C" {
#include <sys/types.h>
#include <libgte.h>
}

#include "game/video.h"
#include "game/bruce.h"
#include "game/trickob.h"
#include "game/m3d.h"
#include "game/colij.h"
#include "game/career.h"

extern SVideoInfo GVInfo1, GVInfo2;
extern int PosBits[], EulerBits[], FlipAngleBits[];
extern CBruce *MechList, *GSkater;
extern volatile int Xblanks;
extern CTrickOb *TrickObList;
extern int TrickPlayer;
extern CBruce *pCurrentPlayer;
extern int PowerUpList;
extern SStats Stats[];

int ReplaySoundLockout;

int GetBit(SVideoInfo *);
int GetNBitsSigned(SVideoInfo *, int);
u_long GetNBitsUnsigned(SVideoInfo *, int);
extern "C" MATRIX *FUN_80095e50(SVECTOR *, MATRIX *);
extern "C" void FUN_8008eebc(int);
void M3d_BuildTransform(CSuper *);
int SFX_PlayX(u_int, short, int);
void SFX_Stop(u_int);
Item *Spool_FindEnviroItem(u_int);
u_short *Trig_GetLinksPointer(int);
void Trig_SendPulse(u_short *);
void M3dColij_InitLineInfo(SLineInfo *);
void Trig_ResetCPCollisionFlags(int);
void Trig_ResetCPExecutedFlags(int);
void M3dZone_LineToItem(SLineInfo *, int);
CBody *M3dColij_LineToSphere(const CVector &, const CVector &, CVector &, int, int);

void GetDataChunk(CBruce *pSkater, int NChunks) {
	int i;
	int DeltaX, DeltaY, DeltaZ;
	SVideoInfo *VInfo;

	if (pSkater == MechList)
		VInfo = &GVInfo1;
	else
		VInfo = &GVInfo2;

	pSkater->mOldPos = pSkater->mPos;

	for (i = 0; i < 6; i++)
		if (!GetBit(VInfo))
			break;

	pSkater->mPos.vx =
	    VInfo->OldPos.vx * 2 - VInfo->OldPos2.vx + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->mPos.vy =
	    VInfo->OldPos.vy * 2 - VInfo->OldPos2.vy + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->mPos.vz =
	    VInfo->OldPos.vz * 2 - VInfo->OldPos2.vz + GetNBitsSigned(VInfo, PosBits[i]);
	VInfo->OldPos2 = VInfo->OldPos;
	VInfo->OldPos = pSkater->mPos;
	pSkater->mPos <<= 12;
	pSkater->mVel = pSkater->mPos - pSkater->mOldPos;

	for (i = 0; i < 6; i++)
		if (!GetBit(VInfo))
			break;

	DeltaX = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	DeltaY = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	DeltaZ = GetNBitsSigned(VInfo, EulerBits[i]) & 0xfff;
	VInfo->Euler.vx = (VInfo->OldEuler.vx + DeltaX + 0x4000) & 0xfff;
	VInfo->Euler.vy = (VInfo->OldEuler.vy + DeltaY + 0x4000) & 0xfff;
	VInfo->Euler.vz = (VInfo->OldEuler.vz + DeltaZ + 0x4000) & 0xfff;
	VInfo->OldEuler = VInfo->Euler;

	if (NChunks == 1) {
		FUN_80095e50(&VInfo->Euler, &pSkater->mRotMatrix);

		pSkater->mRotMatrix.m[0][1] = -pSkater->mRotMatrix.m[0][1];
		pSkater->mRotMatrix.m[1][1] = -pSkater->mRotMatrix.m[1][1];
		pSkater->mRotMatrix.m[2][1] = -pSkater->mRotMatrix.m[2][1];
		pSkater->FRUVectorsFromRotMatrix();
		M3d_BuildTransform((CSuper *)pSkater);
	}

	if (GetBit(VInfo) == 1)
		pSkater->mPhysicsState = (EPhysicsState)GetNBitsUnsigned(VInfo, 4);

	if (GetBit(VInfo) == 1)
		pSkater->mAnim = GetNBitsUnsigned(VInfo, 7);

	VInfo->OldAnim = pSkater->mAnim;

	if (!GetBit(VInfo))
		pSkater->mFrame = VInfo->OldFrame + GetNBitsUnsigned(VInfo, 2);
	else
		pSkater->mFrame = GetNBitsUnsigned(VInfo, 8);

	VInfo->OldFrame = pSkater->mFrame;

	if (GetBit(VInfo))
		pSkater->mExtraFlags |= 2;
	else
		pSkater->mExtraFlags &= ~2;

	if (!GetBit(VInfo)) {
		VInfo->OldFlipAngle = 0;
		VInfo->FlipAngleDelta = 0;
		pSkater->m724 = 0;
		pSkater->m720 = 0;
	} else {
		pSkater->m720 = 1000;

		for (i = 0; i < 6; i++)
			if (!GetBit(VInfo))
				break;

		if (FlipAngleBits[i]) {
			int Delta = GetNBitsSigned(VInfo, FlipAngleBits[i]);

			pSkater->m724 = VInfo->OldFlipAngle + VInfo->FlipAngleDelta + Delta;
			VInfo->OldFlipAngle = pSkater->m724;
			VInfo->FlipAngleDelta = Delta;
		}
	}

	VInfo->Yblanks += GetNBitsUnsigned(VInfo, 2) + 2;

	if (NChunks == 1) {
		int n = VInfo->Yblanks - Xblanks;

		for (i = 0; i < n; i++)
			FUN_8008eebc(0);
	}

	if (GetBit(VInfo)) {
		if (GetBit(VInfo)) {
			int SoundNum = GetNBitsUnsigned(VInfo, 8);

			if (!ReplaySoundLockout)
				SFX_PlayX(SoundNum, 0x2000, 0);
		} else {
			int SoundNum = GetNBitsUnsigned(VInfo, 8);

			if (!ReplaySoundLockout)
				SFX_Stop(SoundNum);
		}
	}

	if (GetBit(VInfo)) {
		if (GetBit(VInfo))
			pSkater->BloodSplat(GetNBitsUnsigned(VInfo, 2), 50);

		if (GetBit(VInfo)) {
			int TrickObIndex = GetNBitsUnsigned(VInfo, 8);
			CTrickOb *pTrickOb = TrickObList;

			for (int i = 0; i < TrickObIndex; i++)
				pTrickOb = pTrickOb->mpNext;

			pTrickOb->mpItem = Spool_FindEnviroItem(pTrickOb->mChecksum);
			pTrickOb->mpItem->Flags |= 0x400;
			pTrickOb->mpItem->RGB = 0x202020;
			pTrickOb->mState = 1;

			if (pSkater == MechList)
				pTrickOb->mPlayer = 1;
			else
				pTrickOb->mPlayer = 2;

			TrickPlayer = pTrickOb->mPlayer;
			Trig_SendPulse(Trig_GetLinksPointer(pTrickOb->mNode));
		}
	}

	if (VInfo->OldPos2.vx || VInfo->OldPos2.vy || VInfo->OldPos2.vz) {
		SLineInfo LInfo;
		int Player;
		CVector nearest;
		CVector Start;
		CVector End;

		LInfo.StartCoords.vx = VInfo->OldPos2.vx << 12;
		LInfo.StartCoords.vy = VInfo->OldPos2.vy << 12;
		LInfo.StartCoords.vz = VInfo->OldPos2.vz << 12;
		LInfo.EndCoords.vx = VInfo->OldPos.vx << 12;
		LInfo.EndCoords.vy = VInfo->OldPos.vy << 12;
		LInfo.EndCoords.vz = VInfo->OldPos.vz << 12;
		M3dColij_InitLineInfo(&LInfo);

		LInfo.RecordTriggerZoneHits = 1;
		Player = 2;

		if (pSkater == GSkater)
			Player = 1;

		Trig_ResetCPCollisionFlags(Player);

		pCurrentPlayer = pSkater;
		M3dZone_LineToItem(&LInfo, 1);

		pCurrentPlayer = 0;
		Trig_ResetCPExecutedFlags(Player);

		Start.vx = VInfo->OldPos2.vx << 12;
		Start.vy = VInfo->OldPos2.vy << 12;
		Start.vz = VInfo->OldPos2.vz << 12;
		End.vx = VInfo->OldPos.vx << 12;
		End.vy = VInfo->OldPos.vy << 12;
		End.vz = VInfo->OldPos.vz << 12;
		CBody *pBody = M3dColij_LineToSphere(Start, End, nearest, PowerUpList, 0);

		if (pBody)
			pBody->Die();
	}

	switch (pSkater->mAnim) {
		case 0x36:
			if (pSkater->mFrame >= 0x70)
				goto On;
		case 0x37:
		case 0x2c:
		case 0x2e:
			pSkater->BoardOff();
			break;
		default:
		On:
			pSkater->BoardOn();
	}

	if (Stats[pSkater->mSkaterIndex].ComboID == 0x4000) {
		if (pSkater->mAnim != 0x3f)
			pSkater->mSubObjectDisplayMask |= 0x1000;
		else
			pSkater->mSubObjectDisplayMask &= ~0x1000;
	}

	if (NChunks != 1)
		VInfo->Yblanks = Xblanks;
}
