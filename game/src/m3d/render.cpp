/* M3d_Render__FPv (0x80083838) flags=-fstructure-noalias */
/* C++ (M3D.CPP), from the demo twin + its MAIN.SYM locals (pItem, Position, PosnRelCam, Transform,
   WorldTransform, TempMatrix, pModel, GlobalFaceFlags, yon; block tmp/r/g/b).
   Renders an item list: per item, picks the face flags, renders super items (twice, tinted, when
   flagged), else walks the model's LOD chain by distance, builds the transform and lights, and
   draws the model fast / non-rotated / fully transformed.
   Levers: -fstructure-noalias (Flags stays in a register across the scratchpad stores); the zero
   translation is gte_ldtr on a `register int asm("$0")` (retail's ctc2 $0; gte_ldtr(0,0,0) makes a
   zero pseudo that cse shares with gte_ldbkdir's and costs s7); the light block is one struct
   (back colour at +6/+0xe, matrices at +0x50/+0x70); the 0x800 semi-trans merge needs the mask and
   the new bits in separate temps; the yon test is written `vz - r > (ushort)(Yon / 10)`; the
   0x2186 store is an if/else of two full statements (crossjumped). */
extern "C" {
	#include <sys/types.h>
#include "game/scratchpad.h"
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/ob.h"
#include "game/camera.h"
#include "game/m3dasm.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pc_render3d.h"
#include "pc_video.h"

/* a model's level-of-detail distance: Options > Graphics > Object Distance; past its last one a
   model isn't drawn, except level geometry when drawing the entire level */
#define LOD_DIST(d)    PC_DrawObjectDistance(d)
#define LOD_LAST(pMod) ((pMod)->NextLOD == 0xffff && !keepAll)

#define RGB2GPU(c)    ((((c) & 0xff) << 2) | (((c) & 0xff00) << 5) | (((c) & 0xff0000) << 8))
#define ANGLES_SET(p) (*(int *)&(p)->Angles.vx || (p)->Angles.vz)
#define SCRATCH_S(o)  (*(ushort *)(SCRATCHPAD + (o)))
#define SCRATCH_I(o)  (*(int *)(SCRATCHPAD + (o)))

struct SLights {
	short pad0[3];
	short BackR; /* 0x06 */
	short pad8[3];
	short BackG; /* 0x0e */
	char pad10[0x50 - 0x10];
	MATRIX LightMatrix;  /* 0x50 */
	MATRIX ColourMatrix; /* 0x70 */
};

extern "C" {
	extern SLights FrustumNormals;
	extern SPSXRegion PSXRegion[];
	extern Item *BaddyList;
	extern int Camera_CurrentMikeCamera;
	extern Item *EnviroList;
	extern int EnvRegions;
	extern int EnvRegions_4;
	extern Item *MechList;
	extern int CurrentWaterLevel;
	extern int CurrentWaterLevel2;
	extern int GLevel;
}

ushort DpqMin, DpqShift, M3d_OTPushback, M3d_OTPushback2;
int RefMapClut, RefMapTpage, M3d_FadeColour;
int TestTheWater, Fast, WaterClipValue, M3d_WireframeMode, DpqMax;
short WaterNormal_6;
SCamera *pCurrentCamera;
SViewport *pCurrentViewport;
void *pColourTable;

void M3d_PreprocessWibblyTextures(int);
void RenderSuperItem(Item *);

/* Options > Graphics > Draw Distance = Entire Level (pc_video.h). The PS1 transforms work in 16-bit
   view space: BoundingSpherePreprocessing culls every item more than 32767 from the camera on an
   axis, and the far plane can't go past it. Items that reach beyond FAR_RANGE (not super items)
   are tested against the view here instead, and the native renderer draws them from
   double-precision vertices (R3d_FarBegin). */
#define FAR_RANGE 30000

/* THPS_FAR_RANGE: a smaller one, to compare the two paths on nearer items (debugging) */
static int FarRange() {
	static int range;

	if (!range)
		range = getenv("THPS_FAR_RANGE") ? atoi(getenv("THPS_FAR_RANGE")) : FAR_RANGE;

	return range;
}

/* 1: such an item, in view (rel: its position relative to the camera); -1: out of view or hidden
   (skip: the hiding flags BoundingSpherePreprocessing was given); 0: an ordinary item. */
static int FarItem(Item *pItem, int skip, int *rel) {
	SModel *pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
	SVECTOR *n = (SVECTOR *)&FrustumNormals; /* the view's planes in camera space */
	int r = pModel->Radius >> 12;
	int reach = 0;
	double l[3];
	int i;

	if (pItem->Flags & 0x200) {
		int s = pItem->Scale.vx;

		if (pItem->Scale.vy > s)
			s = pItem->Scale.vy;

		if (pItem->Scale.vz > s)
			s = pItem->Scale.vz;

		r = (int)((long long)r * s >> 12);
	}

	rel[0] = (pItem->Position.vx >> 12) - pCurrentCamera->Position.vx;
	rel[1] = (pItem->Position.vy >> 12) - pCurrentCamera->Position.vy;
	rel[2] = (pItem->Position.vz >> 12) - pCurrentCamera->Position.vz;

	for (i = 0; i < 3; i++)
		if (abs(rel[i]) + r > reach)
			reach = abs(rel[i]) + r;

	if (reach < FarRange())
		return 0;

	if (pItem->Flags & (skip | 0x1000))
		return -1;

	/* in camera space, against the near plane and the four sides (planes 1-5) */
	for (i = 0; i < 3; i++)
		l[i] = ((double)pCurrentCamera->Inverse.m[i][0] * rel[0] +
		        (double)pCurrentCamera->Inverse.m[i][1] * rel[1] +
		        (double)pCurrentCamera->Inverse.m[i][2] * rel[2]) /
		       4096;

	if (l[2] + r < pCurrentViewport->Hither)
		return -1;

	for (i = 2; i < 6; i++)
		if ((n[i].vx * l[0] + n[i].vy * l[1] + n[i].vz * l[2]) / 4096 < -r)
			return -1;

	return 1;
}

/* the diagonal of what the level's items cover: where the fog ends when it is all drawn */
static void MeasureLevel(Item *pItem) {
	int lo[3], hi[3], first = 1, i;

	for (; pItem; pItem = pItem->NextItem) {
		SModel *pModel;
		int c[3], r;

		if (!PSXRegion[pItem->Region].Usable)
			continue;

		pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
		r = pModel->Radius >> 12;
		c[0] = pItem->Position.vx >> 12;
		c[1] = pItem->Position.vy >> 12;
		c[2] = pItem->Position.vz >> 12;

		for (i = 0; i < 3; i++) {
			if (first || c[i] - r < lo[i])
				lo[i] = c[i] - r;

			if (first || c[i] + r > hi[i])
				hi[i] = c[i] + r;
		}

		first = 0;
	}

	if (!first)
		PC_DrawSetLevelSize((int)sqrt((double)(hi[0] - lo[0]) * (hi[0] - lo[0]) +
		                              (double)(hi[1] - lo[1]) * (hi[1] - lo[1]) +
		                              (double)(hi[2] - lo[2]) * (hi[2] - lo[2])));
}

void M3d_Render(void *p) {
	Item *pItem = (Item *)p;
	VECTOR Position;
	VECTOR PosnRelCam;
	MATRIX Transform;
	MATRIX WorldTransform;
	MATRIX TempMatrix;
	SModel *pModel;
	uint GlobalFaceFlags;
	ushort yon;
	int full = PC_DrawFull();
	int far = 0;
	int farSkip = pItem != BaddyList && Camera_CurrentMikeCamera ? 64 : 1;
	int farRel[3];
	double farPos[3];
	int keepAll;

	if (!pItem)
		return;

	geom_ld_mat(pc_geom.llm, &FrustumNormals.LightMatrix);
	geom_ld_mat(pc_geom.lcm, &FrustumNormals.ColourMatrix);
	geom_load_vector(pc_geom.bk, FrustumNormals.BackR >> 1, FrustumNormals.BackG >> 1, 0);

	if (pItem != BaddyList)
		M3dAsm_BoundingSpherePreprocessing(pItem, Camera_CurrentMikeCamera ? 64 : 1);
	else
		M3dAsm_BoundingSpherePreprocessing(pItem, 1);
	/* the entire level: also the level geometry its scripts hide (Downhill Jam shows each section
	   of the course only as the skater nears it), and none of it dropped beyond its last level of
	   detail */
	keepAll = full && pItem == EnviroList;

	if (keepAll) {
		farSkip = 0;
		M3dAsm_BoundingSpherePreprocessing(pItem, 0);
	}

	if (pItem == EnviroList) {
		M3d_PreprocessWibblyTextures(EnvRegions);
		M3d_PreprocessWibblyTextures(EnvRegions_4);
		if (full)
			MeasureLevel(pItem);
	}

	/* Options > Graphics > Draw Distance and Fog */
	SCRATCH_S(0x284) = PC_DrawFogMin(DpqMin, DpqMax);
	SCRATCH_S(0x294) = PC_DrawFogShift(DpqMin, DpqMax, (short)DpqShift);
	SCRATCH_S(0x2b4) = M3d_OTPushback;
	SCRATCH_S(0x2c4) = M3d_OTPushback2;
	SCRATCH_S(0x2d4) = 0;
	SCRATCH_I(0x364) = RefMapClut;
	SCRATCH_I(0x374) = RefMapTpage;
	SCRATCH_I(0x354) = M3d_FadeColour;

	while (pItem) {
		far = 0;

		if (full && !(pItem->Flags & 2) && PSXRegion[pItem->Region].Usable)
			far = FarItem(pItem, farSkip, farRel);

		if (far < 0 || (!far && (pItem->Flags & 0x8000)))
			goto next;

		if (getenv("THPS_TRACE_CULL")) {
			static uchar prev_active[64];
			static int prev_valid;
			uchar reg = pItem->Region;
			uchar now = PSXRegion[reg].Usable != 0;
			if (reg < 64) {
				if (prev_valid && prev_active[reg] != now) {
					extern volatile unsigned pc_heartbeat;
					fprintf(stderr, "region f%u region=%u Active %u->%u item=%p\n", pc_heartbeat,
					        reg, prev_active[reg], now, (void *)pItem);
				}
				prev_active[reg] = now;
			}
			prev_valid = 1;
		}

		if (!PSXRegion[pItem->Region].Usable)
			goto next;

		GlobalFaceFlags = 0xffff0000;

		if (pItem->Flags & 0x80) {
			if (pItem->Flags & 0x400)
				GlobalFaceFlags = 0xffff000c;
			else
				GlobalFaceFlags = 0xfff70004;
		} else if (pItem->Flags & 0x400) {
			GlobalFaceFlags = 0xfffb0008;
			SCRATCH_I(0x3e4) = RGB2GPU(pItem->RGB);
		}

		if (pItem->Flags & 0x4000) {
			GlobalFaceFlags |= 2;
			GlobalFaceFlags &= ~0x10000;
			GlobalFaceFlags &= ~0x200000;
		} else {
			GlobalFaceFlags &= ~2;
			GlobalFaceFlags |= 0x10000;
			GlobalFaceFlags |= 0x200000;
		}

		if (pItem->Flags & 0x800) {
			uint m = GlobalFaceFlags & ~0x180;
			uint t = (pItem->SemiTrans & 0x180) | 0x40;

			GlobalFaceFlags = m | t;
		}

		SCRATCH_I(0x3f4) = GlobalFaceFlags;
		TestTheWater = (pItem->Flags >> 3) & 1;
		if (far)
			TestTheWater = 0; /* its vertices aren't in the GTE's 16 bits */

		if (pItem->Flags & 2) {
			if (pItem == MechList)
				WaterNormal_6 = (CurrentWaterLevel >> 12) - pCurrentCamera->Position.vy;
			else
				WaterNormal_6 = (CurrentWaterLevel2 >> 12) - pCurrentCamera->Position.vy;

			RenderSuperItem(pItem);

			if (TestTheWater) {
				int tmp;
				int r;
				int g;
				int b;
				uchar NumShadows;

				tmp = pItem->Flags;
				pItem->Flags = tmp | 0x400;

				if (!GLevel) {
					b = 10;
					g = 13;
					r = 8;
				} else {
					b = 34;
					g = 32;
					r = 32;
				}

				pItem->RGB = (b << 16) | (g << 8) | r;
				WaterClipValue = 0;
				GlobalFaceFlags &= ~0x40000;
				GlobalFaceFlags |= 8;
				SCRATCH_I(0x3e4) = RGB2GPU(pItem->RGB);
				SCRATCH_I(0x3f4) = GlobalFaceFlags;
				NumShadows = ((CSuper *)pItem)->mShadowLights;
				((CSuper *)pItem)->mShadowLights = 0;
				RenderSuperItem(pItem);

				pItem->Flags = tmp;
				((CSuper *)pItem)->mShadowLights = NumShadows;
				WaterClipValue = 0xffff;
			}

			goto next;
		}

		pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

		if (pModel->Flags & 0x20)
			goto next;

		if (pModel->Flags & 0x200)
			M3d_WireframeMode = 1;

		geom_ld_mat(pc_geom.rot, &pCurrentCamera->View);
		geom_load_vector(pc_geom.tr, 0, 0, 0);

		PosnRelCam.vx = (pItem->Position.vx >> 12) - pCurrentCamera->Position.vx;
		PosnRelCam.vy = (pItem->Position.vy >> 12) - pCurrentCamera->Position.vy;
		PosnRelCam.vz = (pItem->Position.vz >> 12) - pCurrentCamera->Position.vz;
		{
			/* into view space (16 bits per coordinate in) */
			int16_t v[3];
			GeomAcc r;

			geom_ld_lv(v, &PosnRelCam);
			r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
			geom_st_lv(&Position, r.mac);
		}

		if (far) {
			/* PosnRelCam doesn't fit the GTE: the view-space position in full */
			int i;

			for (i = 0; i < 3; i++)
				farPos[i] = ((double)pCurrentCamera->View.m[i][0] * farRel[0] +
				             (double)pCurrentCamera->View.m[i][1] * farRel[1] +
				             (double)pCurrentCamera->View.m[i][2] * farRel[2]) /
				            4096;

			Position.vx = (int)floor(farPos[0]);
			Position.vy = (int)floor(farPos[1]);
			Position.vz = (int)floor(farPos[2]);
		}

		if (pModel->zMax < 0) {
			while (LOD_DIST(-((short)(pModel->zMax * pCurrentViewport->Zoom / 191) * DpqMax >>
			                  10)) < Position.vz &&
			       pModel->NextLOD != 0xffff)
				pModel = PSXRegion[pItem->Region].ppModels[pModel->NextLOD];

			if (pModel->zMax < 0) {
				if (LOD_DIST(-((short)(pModel->zMax * pCurrentViewport->Zoom / 191) * DpqMax >>
				               10)) < Position.vz &&
				    LOD_LAST(pModel)) {
					if (getenv("THPS_TRACE_CULL")) {
						extern volatile unsigned pc_heartbeat;
						fprintf(
						    stderr,
						    "lodskip(neg) f%u item=%p model=%p LODDist=%d vz=%d thresh=%d\n",
						    pc_heartbeat, (void *)pItem, (void *)pModel, pModel->zMax, Position.vz,
						    -((short)(pModel->zMax * pCurrentViewport->Zoom / 191) * DpqMax >> 10));
					}
					goto next;
				}
			} else {
				if (LOD_DIST(pModel->zMax * pCurrentViewport->Zoom / 191) < Position.vz &&
				    LOD_LAST(pModel)) {
					if (getenv("THPS_TRACE_CULL")) {
						extern volatile unsigned pc_heartbeat;
						fprintf(stderr,
						        "lodskip(pos) f%u item=%p model=%p LODDist=%d vz=%d thresh=%d\n",
						        pc_heartbeat, (void *)pItem, (void *)pModel, pModel->zMax,
						        Position.vz, pModel->zMax * pCurrentViewport->Zoom / 191);
					}
					goto next;
				}
			}
		} else {
			while (LOD_DIST(pModel->zMax * pCurrentViewport->Zoom / 191) < Position.vz &&
			       pModel->NextLOD != 0xffff)
				pModel = PSXRegion[pItem->Region].ppModels[pModel->NextLOD];

			if (LOD_DIST(pModel->zMax * pCurrentViewport->Zoom / 191) < Position.vz &&
			    LOD_LAST(pModel))
				goto next;
		}

		if (pModel->NumVertices < 101 && !(pModel->Flags & 0x100) && !(pItem->Flags & 0x480) &&
		    Position.vz - (pModel->Radius >> 12) > (ushort)(pCurrentViewport->Yon / 10) &&
		    !(pItem->Angles.vx | pItem->Angles.vy | pItem->Angles.vz))
			Fast = 1;
		else
			Fast = 0;

		if (far)
			Fast = 0;

		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr,
			        "render f%u item=%p model=%p verts=%d vz=%d radius=%d yon10=%d Fast=%d\n",
			        pc_heartbeat, (void *)pItem, (void *)pModel, pModel->NumVertices, Position.vz,
			        pModel->Radius >> 12, (ushort)(pCurrentViewport->Yon / 10), Fast);
		}

		if (ANGLES_SET(pItem)) {
			M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &WorldTransform);

			if (pItem->Flags & 0x200)
				M3dMaths_ScaleMatrix(pItem, &WorldTransform);

			M3dMaths_MulRotMatrix(&WorldTransform, &Transform);
			geom_ld_mat(pc_geom.rot, &Transform);
		} else {
			M3dMaths_SetIdentityRotation(&WorldTransform);

			if (pItem->Flags & 0x200) {
				M3dMaths_ScaleMatrix(pItem, &WorldTransform);
				M3dMaths_MulRotMatrix(&WorldTransform, &Transform);
				geom_ld_mat(pc_geom.rot, &Transform);
			}
		}

		pColourTable = PSXRegion[pItem->Region].pColourTable;

		if ((pModel->Flags & 1) || (pItem->Flags & 0x4000)) {
			M3dAsm_GetReflectionMapping(&WorldTransform, (int *)&Position, (uint *)pModel);
		} else if ((pModel->Flags & 4) || (pItem->Flags & 0x80)) {
			if (ANGLES_SET(pItem)) {
				MulMatrix0((MATRIX *)pItem->pLight, &WorldTransform, &TempMatrix);
				geom_ld_mat(pc_geom.llm, &TempMatrix);
			} else {
				geom_ld_mat(pc_geom.llm, pItem->pLight);
			}

			geom_ld_mat(pc_geom.lcm, (char *)pItem->pLight + 20);
			M3dAsm_SetAmbient((int *)((char *)pItem->pLight + 40));
			M3dAsm_GetDynamicLighting((SVECTOR *)((char *)pModel + (pModel->NumVertices * 8 + 28)),
			                          pModel->NumNormals);
		}

		yon = pCurrentViewport->Yon;

		if (pItem->Flags & 0x2000) {
			SCRATCH_S(0x284) = 0x7fff;
			SCRATCH_S(0x2d4) = 0;
			pCurrentViewport->Yon = 0x7fff;
		}

		if (Fast) {
			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				RenderModelFast(pModel, &PosnRelCam);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				RenderModelFast(pModel, &PosnRelCam);
			}
		}
		else if (!far && !ANGLES_SET(pItem) && !(pItem->Flags & 0x200)) {
			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				RenderModelNonRotated(pModel, &PosnRelCam);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				RenderModelNonRotated(pModel, &PosnRelCam);
			}
		} else {
			geom_ld_lv32(pc_geom.tr, &Position);

			if (far) {
				double m[3][3];
				int i, j;

				/* the view matrix times the item's rotation (and scale) */
				for (i = 0; i < 3; i++)
					for (j = 0; j < 3; j++)
						m[i][j] = ((double)pCurrentCamera->View.m[i][0] * WorldTransform.m[0][j] +
						           (double)pCurrentCamera->View.m[i][1] * WorldTransform.m[1][j] +
						           (double)pCurrentCamera->View.m[i][2] * WorldTransform.m[2][j]) /
						          (4096.0 * 4096.0);

				R3d_FarBegin(m, farPos,
				             (pItem->Flags & 0x2000) ? 0x7fffffff : PC_DrawFogStart(DpqMin, DpqMax),
				             PC_DrawFogShift(DpqMin, DpqMax, (short)DpqShift));
			}

			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				RenderModel(pModel);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				RenderModel(pModel);
			}

			if (far)
				R3d_FarEnd();
		}

		if (pItem->Flags & 0x2000) {
			SCRATCH_S(0x2d4) = 0;
			SCRATCH_S(0x284) = PC_DrawFogMin(DpqMin, DpqMax);
			pCurrentViewport->Yon = yon;
		}
	next:
		pItem = pItem->NextItem;
	}
}
