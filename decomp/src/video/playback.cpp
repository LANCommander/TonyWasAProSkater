/* Video_Playback__FP6CBruce (0x8006a930) */
/* C++ (VIDEO.CPP). Fuzzy demo twin; locals and blocks from its MAIN.SYM. THPS1's version calls
   Platform_HandleBounce with no argument. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/video.h"
#include "game/bruce.h"
#include "game/bit.h"

extern SVideoInfo GVInfo1[2];
extern CBruce *MechList;
extern int Status;
extern int BitCount;
extern int UseCacheForBits;
extern int GNumberOfPlayers;

int GVideoReplay;

extern "C" void CamPt_SetClosestCamera__FP6CBruce(CBruce *);
void RestartCompFrig();
extern "C" void Video_Start__FP6CBruce10EVideoModeii(CBruce *, int, int, int);
extern "C" void GetDataChunk__FP6CBrucei(CBruce *, int);
extern "C" int Mips_DotProd2(int, int, int, int);
extern "C" void Bit_SetSparkTrajectoryCone__FRC8CSVector(const CSVector &);
extern "C" void Platform_HandleBounce__Fv();
extern "C" void Front_SwitchOnVideoScreen__Fv();
void Bit_SetSparkTrajectory(const CSVector &);
void Video_SyncObsIfNeeded();
void Video_Stop(CBruce *);

#define ABS(x) (((x) ^ ((x) >> 31)) - ((x) >> 31))

void Video_Playback(CBruce *pSkater) {
	SVideoInfo *VInfo;

	CamPt_SetClosestCamera__FP6CBruce(pSkater);

	if (pSkater == MechList)
		VInfo = &GVInfo1[0];
	else
		VInfo = &GVInfo1[1];

	if (Status == 20 || Status == 22 || Status == 21) {
		if (!VInfo->FramesLeft) {
			RestartCompFrig();
			Video_Start__FP6CBruce10EVideoModeii(pSkater, 2, 1, 0x7fffffff);
		}
	}

	if (VInfo->FramesLeft) {
		int IsAGrind;

		GetDataChunk__FP6CBrucei(pSkater, 1);

		VInfo->FramesLeft--;
		IsAGrind = ABS(Mips_DotProd2(-pSkater->mFront.vz, pSkater->mVel.vx, pSkater->mFront.vx,
		                             pSkater->mVel.vz)) < 40000;
		int st = pSkater->mPhysicsState;

		if (st == 4 && (IsAGrind || pSkater->mAnim == 58) && BitCount < 200) {
			CSVector Angles;

			{
				short x = VInfo->Euler.vx, y = VInfo->Euler.vy, z = VInfo->Euler.vz;

				Angles.vx = x;
				Angles.vy = y;
				Angles.vz = z;
			}

			Bit_SetSparkTrajectory(Angles);
			CSVector cone;

			cone.vx = 450;
			cone.vy = 900;
			cone.vz = 0;
			Bit_SetSparkTrajectoryCone__FRC8CSVector(cone);

			UseCacheForBits = 0;
			CVector SparkPos;

			/* retail stores vz, vx, vy (not the header ctor's order) */
			SparkPos.vz = 0;
			SparkPos.vx = 0;
			SparkPos.vy = 0x19000;

			for (int i = 0; i < 9; i++)
				new CGrindSpark((CBody *)pSkater, SparkPos, -40, 4096, 5);

			st = pSkater->mPhysicsState;
			UseCacheForBits = 1;
		}

		if (st || ABS(pSkater->mPos.vy - pSkater->m2E4) > 64) {
			pSkater->m2E4 = pSkater->mPos.vy;
			pSkater->DoShadowCheck();
		} else {
			pSkater->mShadowPos.vx = pSkater->mPos.vx;
			pSkater->mShadowPos.vz = pSkater->mPos.vz;
		}

		pSkater->GlowUpdate();
		Platform_HandleBounce__Fv();
		pSkater->HandleWater();
	} else {
		if ((uint)(VInfo->ReplayHighlight - 1) < 4 &&
		    VInfo->Highlights[VInfo->ReplayHighlight].Score) {
			int StartFrame = VInfo->Highlights[VInfo->ReplayHighlight].FromFrame;
			int EndFrame = VInfo->Highlights[VInfo->ReplayHighlight].ToFrame;

			if (EndFrame != 0x7fffffff)
				VInfo->FramesLeft = EndFrame - StartFrame + 1;

			{
				int n = GVideoReplay;

				VInfo->ReplayHighlight++;
				GVideoReplay = n + 1;
			}

			Video_SyncObsIfNeeded();
		} else {
			Video_Stop(pSkater);

			if (GNumberOfPlayers == 1 || pSkater == MechList)
				Front_SwitchOnVideoScreen__Fv();
		}
	}
}
