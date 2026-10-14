/* Decomp_GetAnimTransform__FP6CSuper (0x80013cd4) */
/* C++ (DECOMP.CPP). Fuzzy demo twin (0.74, opcode-identical in shape); locals from its MAIN.SYM.
   Returns the per-model matrices for the object's current animation frame. Type-42 files hold
   ready matrices. Type-44 files are compressed Euler/pivot streams: the first call allocates a
   buffer and works out a parent-before-child model order, then each new animation decompresses
   its streams and each new frame rebuilds the matrices down the hierarchy. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/m3dasm.h"
#include "game/mem.h"
#include "game/ob.h"

extern SPSXRegion PSXRegion[];
extern SMatrix TweenBuffer[];

void M3dUtils_InBetween(CSuper *);
uchar *DecompressStream(uchar *, short *, int, int);

SMatrix *Decomp_GetAnimTransform(CSuper *pSuper) {
	SPSXRegion *pRegion;
	uint *pAnimFile;
	uint NumModels;
	uint NumItems;

	pRegion = &PSXRegion[pSuper->mRegion];
	pAnimFile = pRegion->pAnimFile;
	NumModels = ((uint *)pRegion->ppModels)[-1];
	NumItems = pRegion->pPSX[2];

	switch (pAnimFile[-2]) {
		case 42:
			if (*(ushort *)((char *)pAnimFile + pSuper->mAnim * 8 + 10)) {
				M3dUtils_InBetween(pSuper);
				return TweenBuffer;
			}

			return (SMatrix *)((char *)pAnimFile + pAnimFile[pSuper->mAnim * 2 + 1]) +
			       pSuper->mFrame * NumItems;
		case 44: {
			int NewBuffer = 0;
			uint *pAnimHeader;
			int NewAnim;

			if (!pSuper->mpDecompressedFrame) {
				{
					uint *pHeader = pAnimFile + 1;
					int NumAnims = pAnimFile[0];
					uint MaxFrames = 0;
					int i;

					for (i = 0; i < NumAnims; i++) {
						pHeader++;

						if (MaxFrames < *pHeader)
							MaxFrames = *pHeader;

						pHeader++;
					}

					pSuper->mpDecompressedFrame = (SMatrix *)Mem_NewX(
					    NumModels * sizeof(SMatrix) + MaxFrames * NumModels * sizeof(SEulerPivot),
					    1, 1);
				}

				pSuper->mpCalculationOrder = (ushort *)Mem_NewX(NumModels * 2, 0, 1);
				pSuper->mRoot = 0xffff;

				{
					uint i;

					for (i = 0; i < NumModels; i++)
						if (pRegion->pHierarchy[i] == i)
							pSuper->mRoot = i;
				}

				pSuper->mpCalculationOrder[0] = pSuper->mRoot;

				{
					ushort *pParent = pSuper->mpCalculationOrder;
					ushort *pChild = pParent + 1;
					int c = NumModels - 1;

					while (c) {
						ushort *pHi = pRegion->pHierarchy;
						uint i;

						for (i = 0; i < NumModels; i++, pHi++) {
							if (i != pSuper->mRoot && *pHi == *pParent) {
								*pChild++ = i;
								c--;
							}
						}

						pParent++;
					}
				}

				NewBuffer = 1;
			}

			NewAnim = 0;
			pAnimHeader = &pAnimFile[pSuper->mAnim * 2 + 1];

			if (pSuper->mDecompressedAnim != pSuper->mAnim || NewBuffer) {
				SEulerPivot *pEulerPivot = (SEulerPivot *)(pSuper->mpDecompressedFrame + NumModels);
				uchar *pCompressedStream = (uchar *)pAnimFile + pAnimHeader[0];
				int NumFrames = pAnimHeader[1];
				int StepSize = NumModels * 6;
				uint m;

				for (m = 0; m < NumModels; m++) {
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->vx, StepSize, NumFrames);
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->vy, StepSize, NumFrames);
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->vz, StepSize, NumFrames);
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->px, StepSize, NumFrames);
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->py, StepSize, NumFrames);
					pCompressedStream =
					    DecompressStream(pCompressedStream, &pEulerPivot->pz, StepSize, NumFrames);
					pEulerPivot++;
				}

				pSuper->mDecompressedAnim = pSuper->mAnim;
				NewAnim = 1;
			}

			if ((signed char)pSuper->mDecompressedFrame != pSuper->mFrame || NewAnim) {
				SEulerPivot *pEulerPivots =
				    (SEulerPivot *)(pSuper->mpDecompressedFrame + NumModels);
				SMatrix *pMat;
				SEulerPivot *pRootEuler;

				pEulerPivots += NumModels * pSuper->mFrame;
				pMat = pSuper->mpDecompressedFrame + pSuper->mRoot;
				pRootEuler = pEulerPivots + pSuper->mRoot;
				ushort *pModel;
				uint i;

				M3dMaths_RotMatrixYXZ((SVECTOR *)pRootEuler, (MATRIX *)pMat);

				pMat->t[0] = pRootEuler->px;
				pMat->t[1] = pRootEuler->py;
				pMat->t[2] = pRootEuler->pz;
				pModel = pSuper->mpCalculationOrder + 1;

				for (i = 1; i < NumModels; i++) {
					int m = *pModel++;
					SMatrix *pParentMat;
					SEulerPivot *pEulerPivot;

					pMat = pSuper->mpDecompressedFrame + m;
					pEulerPivot = pEulerPivots + m;
					pParentMat = pSuper->mpDecompressedFrame + pRegion->pHierarchy[m];
					M3dMaths_RotMatrixYXZ((SVECTOR *)pEulerPivot, (MATRIX *)pMat);

					{
						/* the pivot through the parent's rotation (>> 12, 16-bit results) */
						GeomAcc r;

						geom_ld_mat(pc_geom.rot, pParentMat);
						r = geom_mvmva(pc_geom.rot, 0, pEulerPivot->px, pEulerPivot->py,
						               pEulerPivot->pz, 12, 0, 0);
						geom_st_sv(&pMat->t[0], r.ir);
					}

					pMat->t[0] += pParentMat->t[0];
					pMat->t[1] += pParentMat->t[1];
					pMat->t[2] += pParentMat->t[2];
				}

				pSuper->mDecompressedFrame = pSuper->mFrame;
			}

			return pSuper->mpDecompressedFrame;
		}
	}
}
