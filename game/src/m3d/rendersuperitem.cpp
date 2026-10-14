/* RenderSuperItem__FP4Item (0x8008468c) flags=-fstructure-noalias */
/* C++ (M3D.CPP). Fuzzy demo twin (0.83 by opcode); locals from its MAIN.SYM. Renders a "super"
   (multi-part animated) item: builds its camera-space transform (mirrored if flagged, with a finer
   translation when it is close), then for each part multiplies in the part's animation matrix,
   draws it (lit, coloured or plain), and adds the part's ellipsoid and shadows. THPS1 calls
   MulMatrix0 where the demo inlined the GTE sequence, and skips a part's extras up front. */
extern "C" {
	#include <sys/types.h>
#include "game/scratchpad.h"
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/ob.h"
#include "game/camera.h"
#include "game/decomp.h"
#include "game/m3dasm.h"

#include "../m3dasm/lastvec.h"

/* a position (16 bits per coordinate) through the current rotation, >> 12, into three words */
static void ViewPosition(const VECTOR *p, long *out) {
	int16_t v[3];
	GeomAcc r;

	geom_ld_lv(v, p);
	r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
	geom_st_lv(out, r.mac);
}

extern SPSXRegion PSXRegion[];
extern SAnimFrame *QuickAnimLookup[];
extern int CurrentWaterLevel;

int Fast;
SCamera *pCurrentCamera;
int Dewibble;
uint FrameNo;
char *ProjectedVertices;
char *FreeStitchVertex;
SAnimFrame *pColourTable;
int SubObjectMasked;
int TestTheWater;

void RenderModelInSuperFast(SModel *);
void M3dElips_RenderEllipsoid(SModel *, MATRIX *, uint, int, int, Texture *);
void M3dElips_RenderShadow(SModel *, MATRIX *, CSVector *, CVector *, SVECTOR *, CVECTOR);

void RenderSuperItem(Item *pItem) {
	uint NumParts;
	uint Rgb;
	VECTOR Position;
	SMatrix *pAnimTransform;
	MATRIX Transform;
	MATRIX SuperTransform;
	MATRIX WorldTransform;
	MATRIX SuperLightTransform;
	int i;
	int Type;
	SModel *pModel;
	SModel **ppModels;
	SAnimFrame *pAnimTex;
	ushort Scale;
	short OTPushback;
	uint ItemFlags;

	ItemFlags = pItem->Flags;

	if (!PSXRegion[pItem->Region].Fastable || (ItemFlags & 0xc80))
		Fast = 0;
	else
		Fast = 1;

	if (((CSuper *)pItem)->mExtraFlags & 2) {
		((CSuper *)pItem)->mTransform.m[0][0] = -((CSuper *)pItem)->mTransform.m[0][0];
		((CSuper *)pItem)->mTransform.m[1][0] = -((CSuper *)pItem)->mTransform.m[1][0];
		((CSuper *)pItem)->mTransform.m[2][0] = -((CSuper *)pItem)->mTransform.m[2][0];
		*(short *)(SCRATCHPAD + 0x2d4) = 1;
	} else {
		*(short *)(SCRATCHPAD + 0x2d4) = 0;
	}

	geom_ld_mat(pc_geom.rot, &pCurrentCamera->View);
	M3dMaths_MulRotMatrix(&((CSuper *)pItem)->mTransform, &SuperTransform);

	Position.vx = (pItem->Position.vx >> 12) - pCurrentCamera->Position.vx;
	Position.vy = (pItem->Position.vy >> 12) - pCurrentCamera->Position.vy;
	Position.vz = (pItem->Position.vz >> 12) - pCurrentCamera->Position.vz;
	ViewPosition(&Position, SuperTransform.t);

	if (*(short *)&SuperTransform.t[2] < 1000) {
		Dewibble = 1;
		Position.vx = (pItem->Position.vx >> 8) - (pCurrentCamera->Position.vx << 4);
		Position.vy = (pItem->Position.vy >> 8) - (pCurrentCamera->Position.vy << 4);
		Position.vz = (pItem->Position.vz >> 8) - (pCurrentCamera->Position.vz << 4);
		ViewPosition(&Position, SuperTransform.t);
	} else {
		Dewibble = 0;
	}

	ppModels = PSXRegion[pItem->Region].ppModels;
	NumParts = PSXRegion[pItem->Region].NumParts;

	if (ItemFlags & 4)
		pAnimTransform = ((CSuper *)pItem)->mpPoseBuffer;
	else
		pAnimTransform = Decomp_GetAnimTransform((CSuper *)pItem);

	MulMatrix0((MATRIX *)pItem->pLight, &((CSuper *)pItem)->mTransform, &SuperLightTransform);

	Type = ((CSuper *)pItem)->mEllipsoidRGB >> 24;

	if (Type & 127) {
		int Lookup = (Type - 1) & 127;

		if (Lookup > 1)
			Lookup = 1;

		if (Lookup < 0)
			Lookup = 0;

		pAnimTex = QuickAnimLookup[Lookup];
		pAnimTex += FrameNo % ((uint *)pAnimTex)[-1];
		Rgb = ((CSuper *)pItem)->mEllipsoidRGB & 0xffffff;
		Scale = ((CSuper *)pItem)->mEllipsoidScale;
		OTPushback = ((CSuper *)pItem)->mEllipsoidOTPushback;
	}

	FreeStitchVertex = Fast ? (char *)(SCRATCHPAD + 0x3f8) : ProjectedVertices + 3192;
	pColourTable = (SAnimFrame *)PSXRegion[pItem->Region].pColourTable;

	for (i = 0; i < NumParts; i++, pAnimTransform++) {
		SubObjectMasked = (((CSuper *)pItem)->mSubObjectDisplayMask & (1 << i)) != 0;
		pModel = ppModels[i];
		MulMatrix0(&((CSuper *)pItem)->mTransform, (MATRIX *)pAnimTransform, &WorldTransform);

		if (Dewibble)
			M3dAsm_SetSuperTransformsDewibbled(&SuperTransform, &SuperLightTransform,
			                                   pAnimTransform);
		else
			M3dAsm_SetSuperTransforms(&SuperTransform, &SuperLightTransform, pAnimTransform);

		if ((pModel->Flags & 1) || (pItem->Flags & 0x4000)) {
			M3dAsm_GetReflectionMapping(&WorldTransform, SuperTransform.t, (uint *)pModel);
		} else if ((pModel->Flags & 4) || (ItemFlags & 128)) {
			geom_ld_mat(pc_geom.lcm, (MATRIX *)((char *)pItem->pLight + 20));
			M3dAsm_SetAmbient((int *)((char *)pItem->pLight + 40));
			M3dAsm_GetDynamicLighting((SVECTOR *)((char *)pModel + (pModel->NumVertices * 8 + 28)),
			                          pModel->NumNormals);
		}

		if (!(Type & 128)) {
			if (Fast)
				RenderModelInSuperFast(pModel);
			else
				RenderModelInSuper(pModel);
		}

		if (!SubObjectMasked) {
			int DoEllipsoids = Type & 127;
			int DoShadows = ((CSuper *)pItem)->mShadowLights != 0;

			if (DoEllipsoids || DoShadows) {
				M3dMaths_SetLastVec(&pAnimTransform->t[0]);
				M3dMaths_TransformLastVec((ushort *)&((CSuper *)pItem)->mTransform);
				M3dMaths_GetLastVecSums(&WorldTransform.t[0]);
				WorldTransform.t[0] >>= 4;
				WorldTransform.t[1] >>= 4;
				WorldTransform.t[2] >>= 4;
				WorldTransform.t[0] += pItem->Position.vx >> 12;
				WorldTransform.t[1] += pItem->Position.vy >> 12;
				WorldTransform.t[2] += pItem->Position.vz >> 12;
			}

			if (DoEllipsoids &&
			    (!TestTheWater || WorldTransform.t[1] < (CurrentWaterLevel >> 12) - 40))
				M3dElips_RenderEllipsoid(pModel, &WorldTransform, Rgb, Scale, OTPushback,
				                         pAnimTex->pTexture);

			if (DoShadows) {
				int s;

				for (s = 0; s < ((CSuper *)pItem)->mShadowLights; s++)
					M3dElips_RenderShadow(
					    pModel, &WorldTransform, &((CSuper *)pItem)->mShadowNormal,
					    &((CSuper *)pItem)->mShadowPos, &((CSuper *)pItem)->mpShadowSource[s],
					    ((CSuper *)pItem)->mpShadowRGB[s]);
			}
		}
	}

	if (((CSuper *)pItem)->mExtraFlags & 2) {
		((CSuper *)pItem)->mTransform.m[0][0] = -((CSuper *)pItem)->mTransform.m[0][0];
		((CSuper *)pItem)->mTransform.m[1][0] = -((CSuper *)pItem)->mTransform.m[1][0];
		((CSuper *)pItem)->mTransform.m[2][0] = -((CSuper *)pItem)->mTransform.m[2][0];
	}
}
