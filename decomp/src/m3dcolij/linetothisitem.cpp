/* M3dColij_LineToThisItem__FP4ItemP9SLineInfo (0x80080964) */
/* C++ (M3DCOLIJ.CPP). Fuzzy demo twin (0.97); locals from its MAIN.SYM. Tests the line against one
   item's model. An unrotated, unscaled item just offsets the line start. Otherwise the item's
   matrix is built in Tfm, with the axis-swapped variant for flagged lines. The model's
   vertices are projected (into the scratchpad when small enough) and the faces tested. A hit on a
   trigger face fires that face's node. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/colij.h"
#include "game/m3dasm.h"
#include "game/trig.h"

struct SProjectedVertex;

extern SPSXRegion PSXRegion[];
extern MATRIX Tfm;
extern SProjectedVertex *ProjectedVertices;
extern int OtherEnvIndex;
extern int *EnvModelChecksums[2];

int CollidedTriggerModel;
int TriggerCollisionCheck;

int Spool_GetEnvIndex(int);

static inline void FireNode(int node, int env) {
	OtherEnvIndex = env ^ 1;
	TriggerCollisionCheck = 0;
	Trig_TriggerCommandPoint(node, 1);
}

void M3dColij_LineToThisItem(Item *pItem, SLineInfo *pInfo) {
	VECTOR PosnRelStart;
	SModel *pModel;
	SProjectedVertex *pVertices;

	gte_SetColorMatrix(&pInfo->WorldCst);

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

	if (pModel->NumVertices > 128)
		pVertices = ProjectedVertices;
	else
		pVertices = (SProjectedVertex *)0x1f800000;

	if (!*(int *)&pItem->Angles && !pItem->Angles.vz && !(pItem->Flags & 0x200)) {
		PosnRelStart.vx = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
		PosnRelStart.vy = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
		PosnRelStart.vz = (pItem->Position.vz - pInfo->StartCoords.vz) >> 12;
		__asm__ volatile("ctc2 $0, $5; ctc2 $0, $6; ctc2 $0, $7");
	} else {
		if (pInfo->DropDown) {
			Tfm.t[0] = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
			Tfm.t[1] = (pInfo->StartCoords.vz - pItem->Position.vz) >> 12;
			Tfm.t[2] = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
		} else {
			Tfm.t[0] = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
			Tfm.t[1] = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
			Tfm.t[2] = (pItem->Position.vz - pInfo->StartCoords.vz) >> 12;
			gte_ldlv0(&Tfm.t[0]);
			gte_lcv0();
			gte_stlvnl(&Tfm.t[0]);
		}

		SetTransMatrix(&Tfm);

		PosnRelStart.vx = 0;
		PosnRelStart.vy = 0;
		PosnRelStart.vz = 0;

		if (*(int *)&pItem->Angles || pItem->Angles.vz) {
			M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &Tfm);

			if (pItem->Flags & 0x200)
				M3dMaths_ScaleMatrix(pItem, &Tfm);

			M3dMaths_MulLcmMatrix(&Tfm, &Tfm);
			gte_SetColorMatrix(&Tfm);
		} else {
			M3dMaths_SetIdentityRotation(&Tfm);
			M3dMaths_ScaleMatrix(pItem, &Tfm);
			M3dMaths_MulLcmMatrix(&Tfm, &Tfm);
			gte_SetColorMatrix(&Tfm);
		}
	}

	if (!(M3dAsm_LineColijProcessVertices(pModel, pVertices, pInfo->Length, &PosnRelStart) &
	      0x60f)) {
		CollidedTriggerModel = -1;
		M3dAsm_LineColijProcessFaces(pModel, pVertices, pInfo, pItem);

		if (CollidedTriggerModel != -1) {
			int i = Spool_GetEnvIndex(pItem->Region);

			FireNode(EnvModelChecksums[i][CollidedTriggerModel], i);
		}
	}
}
