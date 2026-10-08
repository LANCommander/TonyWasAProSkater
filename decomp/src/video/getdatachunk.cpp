/* GetDataChunk__FP6CBrucei (0x8006a158) flags=-fstructure-noalias */
/* C++ (VIDEO.CPP). Locals, their order and types come from the demo's MAIN.SYM. */
#include <sys/types.h>
#include <libgte.h>

struct CVector {
	int vx, vy, vz;
	CVector &operator<<=(const int &);
};

CVector operator-(const CVector &, const CVector &);

struct SVideoInfo {
	int VideoMode;
	u_long *pBlockStart, *pStreamStart, *pStreamEnd, *pStream;
	u_long Bits;
	int BitsLeft;
	u_long NumFrames, FramesLeft;
	char pad24[0x78 - 0x24];
	int OldPhysicsState, OldAnim, OldFrame;
	SVECTOR Euler, OldEuler;
	CVector OldPos, OldPos2;
	int OldFlipAngle, FlipAngleDelta, Xblanks, OldXblanks, PlaybackFrozen;
};

struct CBruce {
	char pad0[4];
	CVector Pos;
	char pad10[0x6c - 0x10];
	CVector Vel;
	char pad78[0xf0 - 0x78];
	CVector OldPos;
	char padfc[0x134 - 0xfc];
	u_long Flags;
	char pad138[0x144 - 0x138];
	short Frame;
	ushort Anim;
	char pad148[0x194 - 0x148];
	u_long Flags2;
	char pad198[0x2e8 - 0x198];
	int Terrain;
	char pad2ec[0x720 - 0x2ec];
	int Flipping, FlipAngle;
	char pad728[0x734 - 0x728];
	int PhysicsState;
	char pad738[0x78c - 0x738];
	MATRIX Matrix;
};

struct CSuper;

struct Item {
	ushort Flags;
	char pad2[0x20 - 2];
	int Colour;
};

struct CTrickOb {
	void *vptr;
	u_long mChecksum;
	ushort mNode;
	u_char mState, mPlayer;
	ushort mScore;
	CTrickOb *mpNext;
	Item *mpItem;
};

struct SLineInfo {
	CVector StartCoords, EndCoords;
	char pad18[0x88 - 0x18];
	u_char RecordTriggerZoneHits;
	char pad89[0xa4 - 0x89];
};

class CBody {
  public:
	char pad0[0x48];
	virtual void f0();
	virtual void Hit();
};

struct STerrain {
	char pad0[0xc];
	int Type;
	char pad10[0x154 - 0x10];
};

extern SVideoInfo GVInfo1, GVInfo2;
extern int PosBits[], EulerBits[], FlipAngleBits[];
extern CBruce *MechList, *GSkater;
extern volatile int Xblanks;
extern CTrickOb *TrickObList;
extern int TrickPlayer;
extern CBruce *pCurrentPlayer;
extern int PowerUpList;
extern STerrain Stats[];

int ReplaySoundLockout;

int GetBit(SVideoInfo *);
int GetNBitsSigned(SVideoInfo *, int);
u_long GetNBitsUnsigned(SVideoInfo *, int);
extern "C" MATRIX *FUN_80095e50(SVECTOR *, MATRIX *);
extern "C" void FUN_8008eebc(int);
extern "C" void FRUVectorsFromRotMatrix__6CBruce(CBruce *);
void M3d_BuildTransform(CSuper *);
int SFX_PlayX(u_int, short, int);
void SFX_Stop(u_int);
extern "C" void BloodSplat__6CBruceii(CBruce *, int, int);
Item *Spool_FindEnviroItem(u_int);
u_short *Trig_GetLinksPointer(int);
void Trig_SendPulse(u_short *);
void M3dColij_InitLineInfo(SLineInfo *);
void Trig_ResetCPCollisionFlags(int);
void Trig_ResetCPExecutedFlags(int);
void M3dZone_LineToItem(SLineInfo *, int);
CBody *M3dColij_LineToSphere(const CVector &, const CVector &, CVector &, int, int);
extern "C" void BoardOff__6CBruce(CBruce *);
extern "C" void BoardOn__6CBruce(CBruce *);

void GetDataChunk(CBruce *pSkater, int NChunks) {
	int i;
	int DeltaX, DeltaY, DeltaZ;
	SVideoInfo *VInfo;

	if (pSkater == MechList)
		VInfo = &GVInfo1;
	else
		VInfo = &GVInfo2;

	pSkater->OldPos = pSkater->Pos;

	for (i = 0; i < 6; i++)
		if (!GetBit(VInfo))
			break;

	pSkater->Pos.vx =
	    VInfo->OldPos.vx * 2 - VInfo->OldPos2.vx + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->Pos.vy =
	    VInfo->OldPos.vy * 2 - VInfo->OldPos2.vy + GetNBitsSigned(VInfo, PosBits[i]);
	pSkater->Pos.vz =
	    VInfo->OldPos.vz * 2 - VInfo->OldPos2.vz + GetNBitsSigned(VInfo, PosBits[i]);
	VInfo->OldPos2 = VInfo->OldPos;
	VInfo->OldPos = pSkater->Pos;
	pSkater->Pos <<= 12;
	pSkater->Vel = pSkater->Pos - pSkater->OldPos;

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
		FUN_80095e50(&VInfo->Euler, &pSkater->Matrix);

		pSkater->Matrix.m[0][1] = -pSkater->Matrix.m[0][1];
		pSkater->Matrix.m[1][1] = -pSkater->Matrix.m[1][1];
		pSkater->Matrix.m[2][1] = -pSkater->Matrix.m[2][1];
		FRUVectorsFromRotMatrix__6CBruce(pSkater);
		M3d_BuildTransform((CSuper *)pSkater);
	}

	if (GetBit(VInfo) == 1)
		pSkater->PhysicsState = GetNBitsUnsigned(VInfo, 4);

	if (GetBit(VInfo) == 1)
		pSkater->Anim = GetNBitsUnsigned(VInfo, 7);

	VInfo->OldAnim = pSkater->Anim;

	if (!GetBit(VInfo))
		pSkater->Frame = VInfo->OldFrame + GetNBitsUnsigned(VInfo, 2);
	else
		pSkater->Frame = GetNBitsUnsigned(VInfo, 8);

	VInfo->OldFrame = pSkater->Frame;

	if (GetBit(VInfo))
		pSkater->Flags |= 2;
	else
		pSkater->Flags &= ~2;

	if (!GetBit(VInfo)) {
		VInfo->OldFlipAngle = 0;
		VInfo->FlipAngleDelta = 0;
		pSkater->FlipAngle = 0;
		pSkater->Flipping = 0;
	} else {
		pSkater->Flipping = 1000;

		for (i = 0; i < 6; i++)
			if (!GetBit(VInfo))
				break;

		if (FlipAngleBits[i]) {
			int Delta = GetNBitsSigned(VInfo, FlipAngleBits[i]);

			pSkater->FlipAngle = VInfo->OldFlipAngle + VInfo->FlipAngleDelta + Delta;
			VInfo->OldFlipAngle = pSkater->FlipAngle;
			VInfo->FlipAngleDelta = Delta;
		}
	}

	VInfo->OldXblanks += GetNBitsUnsigned(VInfo, 2) + 2;

	if (NChunks == 1) {
		int n = VInfo->OldXblanks - Xblanks;

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
			BloodSplat__6CBruceii(pSkater, GetNBitsUnsigned(VInfo, 2), 50);

		if (GetBit(VInfo)) {
			int TrickObIndex = GetNBitsUnsigned(VInfo, 8);
			CTrickOb *pTrickOb = TrickObList;

			for (int i = 0; i < TrickObIndex; i++)
				pTrickOb = pTrickOb->mpNext;

			pTrickOb->mpItem = Spool_FindEnviroItem(pTrickOb->mChecksum);
			pTrickOb->mpItem->Flags |= 0x400;
			pTrickOb->mpItem->Colour = 0x202020;
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
			pBody->Hit();
	}

	switch (pSkater->Anim) {
		case 0x36:
			if (pSkater->Frame >= 0x70)
				goto On;
		case 0x37:
		case 0x2c:
		case 0x2e:
			BoardOff__6CBruce(pSkater);
			break;
		default:
		On:
			BoardOn__6CBruce(pSkater);
	}

	if (Stats[pSkater->Terrain].Type == 0x4000) {
		if (pSkater->Anim != 0x3f)
			pSkater->Flags2 |= 0x1000;
		else
			pSkater->Flags2 &= ~0x1000;
	}

	if (NChunks != 1)
		VInfo->OldXblanks = Xblanks;
}
