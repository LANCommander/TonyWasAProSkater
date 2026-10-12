/* CamPt_SetClosestCamera__FP6CBruce (0x8004e268) */
#include "game/camera.h"
#include "game/bruce.h"
#include "game/video.h"
#include "game/m3d.h"

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern CBruce *MechList;
extern unsigned int Xblanks;
extern short CamPtNode[];
extern unsigned int CamModes[];

int CamPtIndex;

extern SViewport *pCurrentViewport;
extern int camDist;
extern int camPitch;
extern int camYOffset;
extern int LastDpqMax;
extern int LastDpqMin;
extern int DpqMax;
extern int DpqMin;

int Utils_LineOfSight(const CVector &, const CVector &);
short *Trig_GetPosition(CVector &, int);
ulong Utils_CrapDist(const CVector &, const CVector &);
int Utils_Dist(const CVector &, const CVector &);
int Redbook_Rnd(int);

void CamPt_SetClosestCamera(CBruce *pSkater) {
	ulong ClosestDist = 0x7fffffff;
	ulong ClosestDist2;
	ulong Dist;
	int Index = -1;
	int Index2;
	CVector CamPos;
	CVector ClosestPos;
	CVector ClosestPos2;
	CVector AdjustedPlayerPos;
	SVideoInfo *VInfo;

	AdjustedPlayerPos = pSkater->mPos;
	AdjustedPlayerPos.vx += pSkater->mUp.vx * 110;
	AdjustedPlayerPos.vy += pSkater->mUp.vy * 110;
	AdjustedPlayerPos.vz += pSkater->mUp.vz * 110;

	if (pSkater == MechList)
		VInfo = &GVInfo1;
	else
		VInfo = &GVInfo2;

	if (Utils_LineOfSight(AdjustedPlayerPos, pSkater->mpCamera->mPos)) {
		VInfo->NoLOS = 0;
	} else if (++VInfo->NoLOS > 10) {
		VInfo->TabuCam = VInfo->CurCameraIndex;
	}

	{
		int i;

		for (i = 0; i < CamPtIndex; i++) {
			if (i == VInfo->TabuCam)
				continue;

			Trig_GetPosition(CamPos, CamPtNode[i]);

			Dist = Utils_CrapDist(pSkater->mPos, CamPos);

			if (Dist < ClosestDist) {
				ClosestDist2 = ClosestDist;
				ClosestDist = Dist;
				Index2 = Index;
				Index = i;
				ClosestPos2 = ClosestPos;
				ClosestPos = CamPos;
			}
		}
	}

	if (Index != VInfo->CurCameraIndex && Index != -1) {
		if (!Utils_LineOfSight(AdjustedPlayerPos, ClosestPos)) {
			if (Index2 != VInfo->CurCameraIndex && Index2 != -1) {
				if (!Utils_LineOfSight(AdjustedPlayerPos, ClosestPos2))
					Index = -1;
				else {
					Index = Index2;
					ClosestPos = ClosestPos2;
					ClosestDist = ClosestDist2;
				}
			} else
				Index = -1;
		}
	}

	if (ClosestDist > 9000)
		Index = -1;

	if (Index != -1 && Index != VInfo->CurCameraIndex) {
		if (Index != VInfo->OldCameraIndex ||
		    Xblanks >= (unsigned int)(VInfo->CamXblanks + 60)) {
			CVector Dummy;
			short *pCam;
			int Type;
			int Param;

			pCam = Trig_GetPosition(Dummy, CamPtNode[Index]);
			Type = pCam[0];
			Param = pCam[1];
			CamModes[0] &= ~0xc00;

			if (Type == 1)
				CamModes[0] |= 0x400;
			else
				CamModes[0] |= 0x800;

			CamModes[0] = (CamModes[0] & ~0xff) + (Redbook_Rnd(5) + 5) * 10;
			CamModes[Index] = CamModes[0];

			if (ClosestDist > 4000) {
				int Val = 10000 - ClosestDist;

				if (Val < 0)
					Val = 0;

				CamModes[0] = (CamModes[0] & ~0xff) + (Val = Val / 200 + 10);
			}

			{
				int t = Xblanks;
				int o = VInfo->CurCameraIndex;

				VInfo->CurCameraIndex = Index;
				VInfo->CamXblanks = t;
				VInfo->OldCameraIndex = o;
			}

			VInfo->CamPos = ClosestPos;

			if (CamModes[Index] & 0x800) {
				pSkater->mpCamera->SetMode(CAMERAMODE_NORMAL);
				pSkater->mpCamera->SetFocus(pSkater);
				pSkater->mpCamera->SetTripod(pSkater);

				VInfo->TrailingCameraType = Param;
			} else if (CamModes[Index] & 0x400) {
				pSkater->mpCamera->SetMode(CAMERAMODE_NO_BIG_AIR);
				pSkater->mpCamera->SetFocus(pSkater);
				pSkater->mpCamera->SetTripodPos(ClosestPos);
			}
		}
	}

	{
		unsigned int *pFlags;
		unsigned int *base = CamModes;

		Index = VInfo->CurCameraIndex;
		pFlags = base + Index;

		if (*pFlags & 0x800) {
			pSkater->mpCamera->extra_YRot = 0;

			switch (VInfo->TrailingCameraType) {
				case 0:
					camDist = 245;
					camPitch = 215;
					pCurrentViewport->TanHalf = 2410;
					pSkater->mpCamera->extra_YRot = 0;
					camYOffset = -70;
					break;
				case 1:
					camDist = 175;
					camPitch = 50;
					pCurrentViewport->TanHalf = 2930;
					pSkater->mpCamera->extra_YRot = 0;
					camYOffset = -17;
					break;
				case 2:
					camDist = 290;
					camPitch = 150;
					pCurrentViewport->TanHalf = 3410;
					pSkater->mpCamera->extra_YRot = 0;
					camYOffset = -108;
					break;
				case 3:
					break;
				case 4:
					camDist = 245;
					camPitch = 215;
					camYOffset = -70;
					pCurrentViewport->TanHalf = 2410;
					pSkater->mpCamera->extra_YRot = 2048;
					break;
				case 5:
					camDist = 195;
					camPitch = 50;
					camYOffset = -17;
					pCurrentViewport->TanHalf = 2930;
					pSkater->mpCamera->extra_YRot = 2048;
					break;
				case 6:
					camDist = 290;
					camPitch = 150;
					camYOffset = -108;
					pCurrentViewport->TanHalf = 3410;
					pSkater->mpCamera->extra_YRot = 2048;
					break;
			}
		} else if (*pFlags & 0x400) {
			if (*pFlags & 0x200) {
				pCurrentViewport->TanHalf = (256 - (*pFlags & 255)) * 10 + 300;
			} else if (*pFlags & 0x100) {
				int Dist;
				int FogDist;

				FogDist = Utils_Dist(pSkater->mPos, VInfo->CamPos);

				if (FogDist > 2400)
					Dist = 2400;
				else
					Dist = FogDist;

				if (Dist < 450)
					Dist = 450;

				DpqMin = FogDist + LastDpqMin;
				DpqMax = DpqMin + (LastDpqMax - LastDpqMin);
				pCurrentViewport->TanHalf = (int)(*pFlags & 255) * 34130 / Dist;
			}
		}
	}
}
