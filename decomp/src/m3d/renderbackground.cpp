/* M3d_RenderBackground__FPv (0x800844b4) flags=-fstrict-aliasing */
/* C++ (M3D.CPP). Demo twin (same size, identical opcodes); locals from its MAIN.SYM. Walks the
   background item list: for each visible item whose region is loaded, loads the camera matrix
   into the GTE, and if the item is rotated composes its rotation (RotMatrix + matrix multiply)
   into the GTE rotation, then renders its model rotated or not. Brackets the walk with a far
   scratchpad Z limit. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}
#include "game/m3d.h"
#include "game/camera.h"

extern SPSXRegion PSXRegion[];

SCamera *pCurrentCamera;
int pColourTable;
ushort DpqMin;

void RenderBackgroundModel(SModel *);
void RenderBackgroundModelNonRotated(SModel *);

extern "C" {
	void FUN_8008b3d4(SVector *, MATRIX *);
	void FUN_8008b254(MATRIX *, MATRIX *);
	void FUN_8008b238(MATRIX *);
}

void M3d_RenderBackground(void *p) {
	Item *pItem;
	VECTOR Position;
	VECTOR PosnRelCam;
	MATRIX Transform;
	MATRIX WorldTransform;
	SModel *pModel;
	uint GlobalFaceFlags;

	pItem = (Item *)p;

	if (!pItem)
		return;

	*(short *)0x1f800284 = 32767;
	*(short *)0x1f8002d4 = 0;
	GlobalFaceFlags = 0xffff0000;

	do {
		if (!(pItem->Flags & 0x8001) && PSXRegion[pItem->Region].Usable) {
			*(uint *)0x1f8003f4 = GlobalFaceFlags;
			gte_SetRotMatrix(&pCurrentCamera->View);

			if (*(int *)&pItem->Angles || pItem->Angles.vz) {
				FUN_8008b3d4(&pItem->Angles, &WorldTransform);
				FUN_8008b254(&WorldTransform, &Transform);
				gte_SetRotMatrix(&Transform);
			} else {
				FUN_8008b238(&WorldTransform);
			}

			pColourTable = (int)PSXRegion[pItem->Region].pColourTable;
			pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

			if (!*(int *)&pItem->Angles && !pItem->Angles.vz)
				RenderBackgroundModelNonRotated(pModel);
			else
				RenderBackgroundModel(pModel);
		}

		pItem = pItem->NextItem;
	} while (pItem);

	*(short *)0x1f800284 = DpqMin;
}
