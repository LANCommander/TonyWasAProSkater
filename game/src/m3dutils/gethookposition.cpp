/* M3dUtils_GetHookPosition__FP6VECTORP6CSuperi (0x8008125c) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/ob.h"

#include "../m3dasm/lastvec.h"

extern SPSXRegion PSXRegion[];
extern SMatrix TweenBuffer[];

SMatrix *Decomp_GetAnimTransform(CSuper *);

void M3dUtils_InterpolateVectors(int, int, uint *, Item *, int, int);
void M3dAsm_TransformTwice(SVector *, SMatrix *, MATRIX *);

void M3dUtils_GetHookPosition(VECTOR *pResult, CSuper *pSuper, int Hook) {
	SMatrix *pAnimTransform;
	int Anim;
	int Part;
	uint *pAnimFile;
	SHook *pHook;

	pHook = PSXRegion[pSuper->mRegion].pHooks + Hook;
	Anim = pSuper->mAnim;
	Part = pHook->Part;
	pAnimFile = PSXRegion[pSuper->mRegion].pAnimFile;

	if (pSuper->mExtraFlags & 2) {
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}

	if (pSuper->mFlags & 4) {
		pAnimTransform = pSuper->mpPoseBuffer + Part;
	} else {
		ushort Interval = ((ushort *)pAnimFile)[Anim * 4 + 5];

		if (Interval) {
			M3dUtils_InterpolateVectors(4, Interval + 1, pAnimFile, (Item *)pSuper, Part,
			                            PSXRegion[pSuper->mRegion].NumParts);

			pAnimTransform = TweenBuffer + Part;
		} else {
			pAnimTransform = Decomp_GetAnimTransform(pSuper) + Part;
		}
	}

	M3dAsm_TransformTwice((SVector *)pHook, pAnimTransform, &pSuper->mTransform);
	M3dMaths_GetLastVecSums(pResult); /* TransformTwice leaves the point there */
	pResult->vx <<= 12;
	pResult->vy <<= 12;
	pResult->vz <<= 12;
	pResult->vx = pSuper->mPos.vx + ((pResult->vx - pSuper->mPos.vx) >> 4);
	pResult->vy = pSuper->mPos.vy + ((pResult->vy - pSuper->mPos.vy) >> 4);
	pResult->vz = pSuper->mPos.vz + ((pResult->vz - pSuper->mPos.vz) >> 4);

	if (pSuper->mExtraFlags & 2) {
		pSuper->mTransform.m[0][0] = -pSuper->mTransform.m[0][0];
		pSuper->mTransform.m[1][0] = -pSuper->mTransform.m[1][0];
		pSuper->mTransform.m[2][0] = -pSuper->mTransform.m[2][0];
	}
}
