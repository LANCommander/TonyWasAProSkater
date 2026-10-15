/* M3D.CPP: the 3D renderer's per-frame passes: render setup (viewport, frustum planes), item
   and background lists, super (multi-part animated) items, the model draw calls, transforms and
   the wibbly-texture and pulsing-colour preprocessing (data structs in game/m3d.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
	#include "game/scratchpad.h"
}

#include "game/m3d.h"
#include "game/camera.h"
#include "pc_render3d.h"
#include "pc_video.h"
#include "game/dbuffer.h"
#include "game/ob.h"
#include "game/m3dasm.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "game/decomp.h"
#include "m3dasm/lastvec.h"
#include "game/bruce.h"
#include "game/bit.h"
#include "game/globals.h"
#include "game/m3delips.h"
#include "game/main.h"
#include "game/mech.h"
#include "game/spool.h"
#include "game/trig.h"
#include <string.h>

void M3dMaths_TransposeMat(ushort *, ushort *);
extern "C" void FUN_8008b68c(MATRIX *src, MATRIX *dst); /* M3dMaths_CopyMat */
void M3d_PreprocessPulsingColours(int);
void M3d_PreprocessWibblyTextures(int);
void RenderSuperItem(Item *);
void RenderBackgroundModel(SModel *);
void RenderModelInSuperFast(SModel *);
void RenderBackgroundModelNonRotated(SModel *);
int uWibble(STexWibVertInfo *pTexWibVertInfo);
int vWibble(STexWibVertInfo *pTexWibVertInfo);

/* the hand-written assembly's vertex and face passes (src/m3dasm) */
int M3dAsm_TransformAndOutcodeItemVertices(SVECTOR *v, int n);               /* 0x80085d28 */
int M3dAsm_TransformAndOutcodeSuperVertices(SVECTOR *v, int n);              /* 0x80085e98 */
int M3dAsm_TransformAndOutcodeSuperVerticesDewibbled(SVECTOR *v, int n);     /* 0x800860cc */
int M3dAsm_TransformAndOutcodeSuperVerticesFast(SVECTOR *v, int n);          /* 0x800862d4 */
int M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled(SVECTOR *v, int n); /* 0x80086404 */
extern "C" void M3dAsm_OutcodeVerticesInWater(int n, int mask);              /* 0x80086500 */
int M3dAsm_TransformAndOutcodeBackgroundVertices(SVECTOR *v, int n);         /* 0x80086590 */
int M3dAsm_TransformAndOutcodeBlockVertices(SVECTOR *v, int n, VECTOR *o);   /* 0x80086d2c */
void M3dAsm_ProcessPolys(uint *faces, SVECTOR *normals, int count);          /* 0x80086f40 */
int M3dAsm_TransformAndOutcodeVerticesFast(SVECTOR *v, int n, VECTOR *o);    /* 0x80089790 */
void M3dAsm_ProcessPolysFast(uint *faces, SVECTOR *normals, int count);      /* 0x800898ac */

/* the frustum planes (M3d_RenderSetup): the ten normals, then the camera-space ones at [10] and
   [14], which M3d_Render loads as the light and colour matrices (SLights) */
extern SVECTOR FrustumNormals[];
extern uint XblanksNow, XblanksThen, FrameNo;
extern int RefMapClut;
extern int RefMapTpage;
extern int RefMapU0V0;
extern SVECTOR WaterNormal;
extern int TestTheWater;
extern int Fast;
extern int WaterClipValue;
extern uint *pColourTable;
extern int Dewibble;
extern int SubObjectMasked;
extern SProjectedVertex *FreeStitchVertex;
extern short FarColor[4]; /* SVECTOR */
extern int BasePhase;
/* Widescreen (platform/pc, pc_gpu_gl.h): a full-width view takes in pc_wide_pad more columns on
   either side at the same zoom (Hor+), so its side planes open up by as much. */
extern "C" int pc_wide_pad;
/* a model's level-of-detail distance: Options > Graphics > Object Distance; past its last one a
   model isn't drawn, except level geometry when drawing the entire level */
#define LOD_DIST(d)    PC_DrawObjectDistance(d)
#define LOD_LAST(pMod) ((pMod)->NextLOD == 0xffff && !keepAll)
#define RGB2GPU(c)    ((((c) & 0xff) << 2) | (((c) & 0xff00) << 5) | (((c) & 0xff0000) << 8))
#define ANGLES_SET(p) (*(int *)&(p)->Angles.vx || (p)->Angles.vz)
#define SCRATCH_S(o)  (*(ushort *)(SCRATCHPAD + (o)))
#define SCRATCH_I(o)  (*(int *)(SCRATCHPAD + (o)))
/* the light block M3d_Render loads from FrustumNormals */
struct SLights {
	short pad0[3];
	short BackR; /* 0x06 */
	short pad8[3];
	short BackG; /* 0x0e */
	char pad10[0x50 - 0x10];
	MATRIX LightMatrix;  /* 0x50 */
	MATRIX ColourMatrix; /* 0x70 */
};
/* Options > Graphics > Draw Distance = Entire Level (pc_video.h). The PS1 transforms work in 16-bit
   view space: BoundingSpherePreprocessing culls every item more than 32767 from the camera on an
   axis, and the far plane can't go past it. Items that reach beyond FAR_RANGE (not super items)
   are tested against the view here instead, and the native renderer draws them from
   double-precision vertices (R3d_FarBegin). */
#define FAR_RANGE 30000
struct SDcWibBase {
	uint *pData;
	uint *pPSX;
	int NumFaces;
	ushort *pUV;
};
static SDcWibBase DcWibBase[64];

/* THPS_FAR_RANGE: a smaller one, to compare the two paths on nearer items (debugging) */
static int FarRange() {
	static int range;

	if (!range)
		range = getenv("THPS_FAR_RANGE") ? atoi(getenv("THPS_FAR_RANGE")) : FAR_RANGE;

	return range;
}

static int PC_ViewWidth(SViewport *pVP) {
	int w = pVP->xR - pVP->xL;

	if (pVP->xL == 0 && pVP->xR == Xres)
		w += 2 * pc_wide_pad;

	return w;
}

/* M3d_RenderSetup__FP7SCameraP9SViewportPUi (0x800831f8) */
/* Demo twin; locals from its MAIN.SYM (Window, pClipArea, opp, adj, hyp, pTex, i).
   Per-frame render setup: region-change count, camera/viewport/OT globals, the clip DR_AREA,
   viewport zoom and centre, the ten frustum plane normals (rotated into camera space with
   ApplyMatrixSV), geometry screen/offset, the scaled view matrix, the sky texture page and the
   pulsing/wibbly texture preprocessing. Levers: -fstructure-noalias; the four camera-up shorts
   at 0x800d2180 are one SVECTOR (a struct, so its stores stay ordered against the camera loads). */
void M3d_RenderSetup(SCamera *pCamera, SViewport *pVP, P_TAG *ot) {
	RECT Window;
	DR_AREA *pClipArea;
	uint opp;
	uint adj;
	uint hyp;
	Texture *pTex;

	XblanksThen = XblanksNow;
	XblanksNow = Xblanks;

	if (XblanksThen != XblanksNow)
		FrameNo++;

	pCurrentCamera = pCamera;
	pCurrentViewport = pVP;
	pVP->Yon = PC_DrawFarPlane((ushort)DpqMax); /* Options > Graphics > Draw Distance */
	CurrentOrderingTable = ot;
	Window.x = 0;
	Window.y = *(ushort *)&pDoubleBuffer->Draw.clip.y;
	Window.w = Xres;
	Window.h = Yres;
	pClipArea = (DR_AREA *)pPoly;
	SetDrawArea(pClipArea, &Window);

	pPoly = (u_char *)(pClipArea + 1);
	addPrim(CurrentOrderingTable, pClipArea);

	pVP->Zoom = ((((pVP->xR - pVP->xL) >> 1) << 12) / pVP->TanHalf << 12) / PixelAspectY;
	pVP->xC = (pVP->xL + pVP->xR) >> 1;
	pVP->yC = (pVP->yT + pVP->yB) >> 1;

	FrustumNormals[0].vx = 0;
	FrustumNormals[0].vy = 0;
	FrustumNormals[0].vz = -4096;
	FrustumNormals[0].pad = pVP->Yon;
	FrustumNormals[1].vx = 0;
	FrustumNormals[1].vy = 0;
	FrustumNormals[1].vz = 4096;
	FrustumNormals[1].pad = -pVP->Hither;

	opp = (pVP->xR - pVP->xL) << 11;
	adj = pVP->Zoom * PixelAspectX;
	hyp = M3dMaths_SquareRoot0((opp >> 12) * (opp >> 12) + (adj >> 12) * (adj >> 12));
	FrustumNormals[2].vx = 0;
	FrustumNormals[2].vy = adj / hyp;
	FrustumNormals[2].vz = opp / hyp;
	FrustumNormals[2].pad = 0;
	FrustumNormals[3].vx = 0;
	FrustumNormals[3].vy = -(adj / hyp);
	FrustumNormals[3].vz = opp / hyp;
	FrustumNormals[3].pad = 0;

	opp = PC_ViewWidth(pVP) << 11;
	adj = pVP->Zoom * PixelAspectY;
	hyp = M3dMaths_SquareRoot0((opp >> 12) * (opp >> 12) + (adj >> 12) * (adj >> 12));
	FrustumNormals[4].vx = adj / hyp;
	FrustumNormals[4].vy = 0;
	FrustumNormals[4].vz = opp / hyp;
	FrustumNormals[4].pad = 0;
	FrustumNormals[5].vx = -(adj / hyp);
	FrustumNormals[5].vy = 0;
	FrustumNormals[5].vz = opp / hyp;
	FrustumNormals[5].pad = 0;

	opp = (pVP->yB - pVP->yT) >> 1;
	adj = pVP->Zoom;
	hyp = M3dMaths_SquareRoot0(opp * opp + adj * adj);
	FrustumNormals[6].vx = 0;
	FrustumNormals[6].vy = (adj << 12) / hyp;
	FrustumNormals[6].vz = (opp << 12) / hyp;
	FrustumNormals[6].pad = 0;
	FrustumNormals[7].vx = 0;
	FrustumNormals[7].vy = -((adj << 12) / hyp);
	FrustumNormals[7].vz = (opp << 12) / hyp;
	FrustumNormals[7].pad = 0;

	opp = PC_ViewWidth(pVP) >> 1;
	adj = pVP->Zoom;
	hyp = M3dMaths_SquareRoot0(opp * opp + adj * adj);
	FrustumNormals[8].vx = (adj << 12) / hyp;
	FrustumNormals[8].vy = 0;
	FrustumNormals[8].vz = (opp << 12) / hyp;
	FrustumNormals[8].pad = 0;
	FrustumNormals[9].vx = -((adj << 12) / hyp);
	FrustumNormals[9].vy = 0;
	FrustumNormals[9].vz = (opp << 12) / hyp;
	FrustumNormals[9].pad = 0;

	{
		int i;

		for (i = 0; i < 3; i++) {
			ApplyMatrixSV(&pCamera->Transform, &FrustumNormals[i], (SVECTOR *)&((SVector *)&FrustumNormals[10])[i]);
			ApplyMatrixSV(&pCamera->Transform, &FrustumNormals[i + 3],
			              (SVECTOR *)&((SVector *)&FrustumNormals[14])[i]);
		}
	}

	SetGeomScreen(pVP->Zoom);
	SetGeomOffset(pVP->xC, pVP->yC);
	M3dMaths_TransposeMat((ushort *)&pCamera->Transform, (ushort *)&pCamera->Inverse);

	{
		int i;

		for (i = 0; i < 3; i++) {
			pCamera->View.m[0][i] = (pCamera->Inverse.m[0][i] * PixelAspectY) >> 12;
			pCamera->View.m[1][i] = (pCamera->Inverse.m[1][i] * PixelAspectX) >> 12;
			pCamera->View.m[2][i] = pCamera->Inverse.m[2][i];
		}
	}

	/* the reflection map's texture (none in the front end: retail read a null entry's fields
	   from low RAM) */
	pTex = Spool_FindTextureEntry(0xcce0187c);

	if (pTex) {
		RefMapClut = pTex->clut << 16;
		RefMapTpage = pTex->tpage << 16;
		RefMapU0V0 = pTex->u0 | (pTex->v0 << 8);
	} else {
		RefMapClut = RefMapTpage = RefMapU0V0 = 0;
	}
	M3d_PreprocessPulsingColours(EnvRegions[0]);
	M3d_PreprocessPulsingColours(EnvRegions[1]);
	M3d_PreprocessPulsingColours(CurrentObjFileRegion);
	M3d_PreprocessWibblyTextures(CurrentObjFileRegion);

	WaterNormal.vx = -pCurrentCamera->Transform.m[1][0];
	WaterNormal.vy = -pCurrentCamera->Transform.m[1][1];
	WaterNormal.vz = -pCurrentCamera->Transform.m[1][2];
	WaterNormal.pad = (CurrentWaterLevel >> 12) - pCurrentCamera->Position.vy;
	R3d_RenderSetup(pVP, ot); /* the native renderer's 3D layer */
}

/* M3d_RenderCleanup__Fv (0x80083774) */
void M3d_RenderCleanup(void) {
	RECT r;
	DR_AREA *p;

	r.x = (short)pCurrentViewport->xL;
	r.y = (short)pCurrentViewport->yT + pDoubleBuffer->Draw.clip.y;
	p = (DR_AREA *)pPoly;
	r.w = (short)pCurrentViewport->xR - (short)pCurrentViewport->xL;
	r.h = (short)pCurrentViewport->yB - (short)pCurrentViewport->yT;
	SetDrawArea(p, &r);

	{
		addPrim(&CurrentOrderingTable[4095], p);
	}

	pPoly = (u_char *)(p + 1);
}

/* 1: such an item, in view (rel: its position relative to the camera); -1: out of view or hidden
   (skip: the hiding flags BoundingSpherePreprocessing was given); 0: an ordinary item. */
static int FarItem(Item *pItem, int skip, int *rel) {
	SModel *pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
	SVECTOR *n = FrustumNormals; /* the view's planes in camera space */
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

/* M3d_Render__FPv (0x80083838) */
/* From the demo twin + its MAIN.SYM locals (pItem, Position, PosnRelCam, Transform,
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
	int farSkip = pItem != (Item *)BaddyList && Camera_CurrentMikeCamera ? 64 : 1;
	int farRel[3];
	double farPos[3];
	int keepAll;

	if (!pItem)
		return;

	geom_ld_mat(pc_geom.llm, &((SLights *)FrustumNormals)->LightMatrix);
	geom_ld_mat(pc_geom.lcm, &((SLights *)FrustumNormals)->ColourMatrix);
	geom_load_vector(pc_geom.bk, ((SLights *)FrustumNormals)->BackR >> 1,
	                 ((SLights *)FrustumNormals)->BackG >> 1, 0);

	if (pItem != (Item *)BaddyList)
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
		M3d_PreprocessWibblyTextures(EnvRegions[0]);
		M3d_PreprocessWibblyTextures(EnvRegions[1]);
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
			if (pItem == (Item *)MechList)
				WaterNormal.pad = (CurrentWaterLevel >> 12) - pCurrentCamera->Position.vy;
			else
				WaterNormal.pad = (CurrentWaterLevel2 >> 12) - pCurrentCamera->Position.vy;

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

/* M3d_RenderBackground__FPv (0x800844b4) */
/* Demo twin (same size, identical opcodes); locals from its MAIN.SYM. Walks the
   background item list: for each visible item whose region is loaded, loads the camera matrix
   into the GTE, and if the item is rotated composes its rotation (RotMatrix + matrix multiply)
   into the GTE rotation, then renders its model rotated or not. Brackets the walk with a far
   scratchpad Z limit. */
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

	*(short *)(SCRATCHPAD + 0x284) = 32767;
	*(short *)(SCRATCHPAD + 0x2d4) = 0;
	GlobalFaceFlags = 0xffff0000;

	do {
		if (!(pItem->Flags & 0x8001) && PSXRegion[pItem->Region].Usable) {
			*(uint *)(SCRATCHPAD + 0x3f4) = GlobalFaceFlags;
			geom_ld_mat(pc_geom.rot, &pCurrentCamera->View);

			if (*(int *)&pItem->Angles || pItem->Angles.vz) {
				M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &WorldTransform);
				M3dMaths_MulRotMatrix(&WorldTransform, &Transform);
				geom_ld_mat(pc_geom.rot, &Transform);
			} else {
				M3dMaths_SetIdentityRotation(&WorldTransform);
			}

			pColourTable = PSXRegion[pItem->Region].pColourTable;
			pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

			if (!*(int *)&pItem->Angles && !pItem->Angles.vz)
				RenderBackgroundModelNonRotated(pModel);
			else
				RenderBackgroundModel(pModel);
		}

		pItem = pItem->NextItem;
	} while (pItem);

	*(short *)(SCRATCHPAD + 0x284) = PC_DrawFogMin(DpqMin, DpqMax);
}

/* a position (16 bits per coordinate) through the current rotation, >> 12, into three words */
static void ViewPosition(const VECTOR *p, int *out) {
	int16_t v[3];
	GeomAcc r;

	geom_ld_lv(v, p);
	r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
	geom_st_lv(out, r.mac);
}

/* RenderSuperItem__FP4Item (0x8008468c) */
/* Fuzzy demo twin (0.83 by opcode); locals from its MAIN.SYM. Renders a "super"
   (multi-part animated) item: builds its camera-space transform (mirrored if flagged, with a finer
   translation when it is close), then for each part multiplies in the part's animation matrix,
   draws it (lit, coloured or plain), and adds the part's ellipsoid and shadows. THPS1 calls
   MulMatrix0 where the demo inlined the GTE sequence, and skips a part's extras up front. */
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

	FreeStitchVertex = Fast ? (SProjectedVertex *)(SCRATCHPAD + 0x3f8)
	                        : (SProjectedVertex *)((char *)ProjectedVertices + 3192);
	pColourTable = PSXRegion[pItem->Region].pColourTable;

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

/* RenderModel__FP6SModel (0x80084d18) */
void RenderModel(SModel *p) {
	SVECTOR *v = p->Vertices;
	int n = p->NumVertices;
	int r = M3dAsm_TransformAndOutcodeItemVertices(v, n);
	SVECTOR *w;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80084d18 f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (TestTheWater)
		M3dAsm_OutcodeVerticesInWater(n, WaterClipValue);

	v += n;

	if (!(r & 0xbf)) {
		n = p->NumFaces;
		w = v + p->NumNormals;
		SetFarColor(FarColor[0], FarColor[1], FarColor[2]);
		M3dAsm_ProcessPolys((uint *)w, v, n);
	}
}

/* RenderModelFast__FP6SModelP6VECTOR (0x80084dc0) */
void RenderModelFast(SModel *p, VECTOR *c) {
	SVECTOR *v = p->Vertices;
	int n = p->NumVertices;
	int r = M3dAsm_TransformAndOutcodeVerticesFast(v, n, c);
	SVECTOR *w;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80084dc0 f%u p=%p n=%d c=%p r=%08x %s\n", pc_heartbeat, (void *)p, n, (void *)c, r,
		        (r & 0x6000e000) ? "CULLED" : "drawn");
	}

	v += n;

	if (!(r & 0x6000e000)) {
		n = p->NumFaces;
		w = v + p->NumNormals;
		SetFarColor(FarColor[0], FarColor[1], FarColor[2]);
		M3dAsm_ProcessPolysFast((uint *)w, v, n);
	}
}

/* RenderBackgroundModel__FP6SModel (0x80084e50) */
void RenderBackgroundModel(SModel *p) {
	SVECTOR *v = p->Vertices;
	int n = p->NumVertices;
	int r = M3dAsm_TransformAndOutcodeBackgroundVertices(v, n);

	v += n;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "bgmodel(rot) f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (!(r & 0xbf)) {
		n = p->NumFaces;
		M3dAsm_ProcessPolys((uint *)(v + p->NumNormals), v, n);
	}
}

/* RenderModelInSuper__FP6SModel (0x80084ebc) */
void RenderModelInSuper(SModel *p) {
	int n = p->NumVertices;
	SVECTOR *v = p->Vertices;
	int r;

	if (Dewibble)
		r = M3dAsm_TransformAndOutcodeSuperVerticesDewibbled(v, n);
	else
		r = M3dAsm_TransformAndOutcodeSuperVertices(v, n);

	if (TestTheWater)
		M3dAsm_OutcodeVerticesInWater(n, WaterClipValue);

	v += n;

	if (!SubObjectMasked && !(r & 0xbf))
		M3dAsm_ProcessPolys((uint *)(v + p->NumNormals), v, p->NumFaces);
}

/* RenderModelInSuperFast__FP6SModel (0x80084f78) */
void RenderModelInSuperFast(SModel *m) {
	int n = m->NumVertices;
	char *p = (char *)m->Vertices;
	int r;

	if (Dewibble)
		r = M3dAsm_TransformAndOutcodeSuperVerticesFastDewibbled((SVECTOR *)p, n);
	else
		r = M3dAsm_TransformAndOutcodeSuperVerticesFast((SVECTOR *)p, n);

	p += n * 8;

	if (SubObjectMasked == 0 && (r & 0x60006000) == 0)
		M3dAsm_ProcessPolysFast((uint *)(p + m->NumNormals * 8), (SVECTOR *)p, m->NumFaces);
}

/* RenderModelNonRotated__FP6SModelP6VECTOR (0x80085018) */
void RenderModelNonRotated(SModel *p, VECTOR *c) {
	SVECTOR *v = p->Vertices;
	int n = p->NumVertices;
	int r = M3dAsm_TransformAndOutcodeBlockVertices(v, n, c);
	SVECTOR *w;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "80085018 f%u p=%p n=%d c=%p r=%08x %s\n", pc_heartbeat, (void *)p, n, (void *)c, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (TestTheWater)
		M3dAsm_OutcodeVerticesInWater(n, WaterClipValue);

	v += n;

	if (!(r & 0xbf)) {
		n = p->NumFaces;
		w = v + p->NumNormals;
		SetFarColor(FarColor[0], FarColor[1], FarColor[2]);
		M3dAsm_ProcessPolys((uint *)w, v, n);
	}
}

/* RenderBackgroundModelNonRotated__FP6SModel (0x800850c4) */
void RenderBackgroundModelNonRotated(SModel *p) {
	SVECTOR *v = p->Vertices;
	int n = p->NumVertices;
	int r = M3dAsm_TransformAndOutcodeBackgroundVertices(v, n);

	v += n;

	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "bgmodel f%u p=%p n=%d r=%08x %s\n", pc_heartbeat, (void *)p, n, r,
		        (r & 0xbf) ? "CULLED" : "drawn");
	}

	if (!(r & 0xbf)) {
		n = p->NumFaces;
		M3dAsm_ProcessPolys((uint *)(v + p->NumNormals), v, n);
	}
}

/* M3d_BuildTransform__FP6CSuper (0x80085130) */
/* C++. A super item's transform (mTransform): a skater's rotation matrix with the y column
   negated (mExtraFlags bit 0), else its angles' rotation; scaled if the item is (flag 0x200);
   translated to its position in model units. */
void M3d_BuildTransform(CSuper *t) {
	MATRIX *m = &t->mTransform;

	if (t->mExtraFlags & 1) {
		FUN_8008b68c(&((CBruce *)t)->mRotMatrix, m);

		m->m[0][1] = -(ushort)m->m[0][1];
		m->m[1][1] = -(ushort)m->m[1][1];
		m->m[2][1] = -(ushort)m->m[2][1];
	} else {
		M3dMaths_RotMatrixYXZ((SVECTOR *)&t->mAngles, m);
	}

	if (t->mFlags & 0x200)
		M3dMaths_ScaleMatrix((Item *)t, m);

	m->t[0] = t->mPos.vx >> 12;
	m->t[1] = t->mPos.vy >> 12;
	m->t[2] = t->mPos.vz >> 12;
}

/* uWibble__FP15STexWibVertInfo (0x800851ec) */
int uWibble(STexWibVertInfo *v) {
	int t = v->uAmpPhase >> 4;
	int o = v->uAmpPhase & 15;

	if (t != 0) {
		int i = (BasePhase + o * 4) & 63;
		short *row = WibbleTables[t];

		return row[i];
	}

	return 0;
}

/* vWibble__FP15STexWibVertInfo (0x8008523c) */
/* C++ (M3D.CPP, between M3d_BuildTransform and M3d_PreprocessWibblyTextures). Looks up a face's
   wibble offset: the high nibble of byte 3 picks a 64-entry row of a short table (0 = none), the
   low nibble phase-shifts the global counter BasePhase into it. */
int vWibble(STexWibVertInfo *p) {
	int hi = p->vAmpPhase >> 4;
	int lo = p->vAmpPhase & 15;

	if (hi) {
		int i = (BasePhase + lo * 4) & 63;
		short *row = WibbleTables[hi];

		return row[i];
	}

	return 0;
}

static void M3d_PreprocessWibblyTexturesDc(int Region, uint *pData, uint *pPSX) {
	SDcWibBase *b = &DcWibBase[Region & 63];
	int total = 0;
	int n;
	uint *p;

	for (p = pData; *p; p = (uint *)((STexWibFaceInfo *)(p + 4) + p[3]))
		total += p[3];

	if (b->pData != pData || b->pPSX != pPSX || b->NumFaces != total) {
		free(b->pUV);

		b->pData = pData;
		b->pPSX = pPSX;
		b->NumFaces = total;
		b->pUV = (ushort *)malloc(total * 16 + 16);
		n = 0;

		for (p = pData; *p; p = (uint *)((STexWibFaceInfo *)(p + 4) + p[3])) {
			Item *pItem = PSX_ITEM(Region, *p);
			SModel *pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
			uint *pFace = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
			int i;

			for (i = 0; i < (int)p[3]; i++, n++) {
				memcpy(b->pUV + n * 8, (char *)pFace + 20, 16);

				pFace = (uint *)((char *)pFace + (((ushort *)pFace)[1] & 0xfffc));
			}
		}
	}

	n = 0;

	for (p = pData; *p; p = (uint *)((STexWibFaceInfo *)(p + 4) + p[3])) {
		Item *pItem = PSX_ITEM(Region, *p);
		STexWibFaceInfo *pInfo = (STexWibFaceInfo *)(p + 4);
		int NumFaces = p[3];
		int i;

		if (pItem->Flags & 0x8000) {
			n += NumFaces;
			continue;
		}

		SModel *pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
		uint *pFace = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
		int uScroll = ((int)XblanksNow * ((short *)p)[2]) >> 4;
		int vScroll = ((int)XblanksNow * ((short *)p)[3]) >> 4;

		BasePhase = ((int)XblanksNow * (int)p[2]) >> 10;

		for (i = 0; i < NumFaces; i++, n++, pInfo++) {
			STexWibVertInfo *v[4] = { &pInfo->p0, &pInfo->p1, &pInfo->p2, &pInfo->p3 };
			ushort *uv = (ushort *)((char *)pFace + 20);
			const ushort *base = b->pUV + n * 8;
			int k;

			for (k = 0; k < 4; k++) {
				uv[k] = base[k] + ((uScroll + uWibble(v[k])) >> 5);
				uv[4 + k] = base[4 + k] + ((vScroll + vWibble(v[k])) >> 5);
			}

			pFace = (uint *)((char *)pFace + (((ushort *)pFace)[1] & 0xfffc));
		}
	}
}

/* M3d_PreprocessWibblyTextures__Fi (0x8008528c) */
/* THPS1's M3d_PreprocessWibblyTextures(Region): the demo twin minus its
   ZeroU/ZeroVAmplitudes skips and the 16-pass cap on the tile-wrap loops, with u/v wibble calls
   interleaved per vertex. vWibble__FP15STexWibVertInfo is vWibble. Locals and line order from the demo SYM.
   Matching notes:
   - The uv3 half of the last word goes through masked block locals (u3Hi/v3Hi) so local-alloc puts
     them in a0/a1 and the or-chain in v0, which lets sched2 hoist them as retail does.
   - The do{}while(0) wrappers around the v2/v3 wibble adds raise their global-alloc refs (40 -> 42)
     to give retail's u1,u0,u2,v3,v2,u3,v1,v0 priority order (s1..fp).
   - The `0xff00U` masks matter: an unsigned mask stops the store to a ushort narrowing the AND
     into `li -256`. */
void M3d_PreprocessWibblyTextures(int Region) {
	uint *pData;
	uint *pPSX;
	uint *pFace;
	STexWibFaceInfo *pTexWibFaceInfo;
	Item *pItem;
	SModel *pModel;
	int NumFaces;
	int i;
	int uScroll;
	int vScroll;
	int TileWidth;
	int TileHeight;
	int u0;
	int v0;
	int u1;
	int v1;
	int u2;
	int v2;
	int u3;
	int v3;
	int And;
	int Or;

	if (Region == -1)
		return;

	pData = PSXRegion[Region].pTexWibData;

	if (!pData)
		return;

	pPSX = PSXRegion[Region].pPSX;

	if ((pPSX[0] & 0xffff) == 6) {
		M3d_PreprocessWibblyTexturesDc(Region, pData, pPSX);
		return;
	}

	while (*pData) {
		pItem = PSX_ITEM(Region, *pData);
		pTexWibFaceInfo = (STexWibFaceInfo *)(pData + 4);
		NumFaces = pData[3];

		if (pItem->Flags & 0x8000) {
			pData = (uint *)(pTexWibFaceInfo + NumFaces);
			continue;
		}

		pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
		pFace = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
		uScroll = ((int)XblanksNow * ((short *)pData)[2]) >> 4;
		vScroll = ((int)XblanksNow * ((short *)pData)[3]) >> 4;
		BasePhase = ((int)XblanksNow * (int)pData[2]) >> 10;

		for (i = 0; i < NumFaces; i++, pTexWibFaceInfo++) {
			TileWidth = ((-(pFace[7] & 0x1f)) << 11) & 0xf800;
			TileHeight = ((-(pFace[7] & 0x3e0)) << 6) & 0xff00;
			u0 = pTexWibFaceInfo->p0.u << 8;
			v0 = pTexWibFaceInfo->p0.v << 8;
			u1 = pTexWibFaceInfo->p1.u << 8;
			v1 = pTexWibFaceInfo->p1.v << 8;
			u2 = pTexWibFaceInfo->p2.u << 8;
			v2 = pTexWibFaceInfo->p2.v << 8;
			u3 = pTexWibFaceInfo->p3.u << 8;
			v3 = pTexWibFaceInfo->p3.v << 8;

			if (uScroll) {
				u0 += uScroll;
				u1 += uScroll;
				u2 += uScroll;
				u3 += uScroll;
			}

			if (vScroll) {
				v0 += vScroll;
				v1 += vScroll;
				v2 += vScroll;
				v3 += vScroll;
			}

			u0 += uWibble(&pTexWibFaceInfo->p0);
			v0 += vWibble(&pTexWibFaceInfo->p0);
			u1 += uWibble(&pTexWibFaceInfo->p1);
			v1 += vWibble(&pTexWibFaceInfo->p1);
			u2 += uWibble(&pTexWibFaceInfo->p2);

			do {
				v2 += vWibble(&pTexWibFaceInfo->p2);
			} while (0);

			u3 += uWibble(&pTexWibFaceInfo->p3);

			do {
				v3 += vWibble(&pTexWibFaceInfo->p3);
			} while (0);

			And = u0 & u1 & u2 & u3;
			Or = u0 | u1 | u2 | u3;

			while ((And ^ Or) & 0x10000) {
				u0 += TileWidth;
				u1 += TileWidth;
				u2 += TileWidth;
				u3 += TileWidth;
				And = u0 & u1 & u2 & u3;
				Or = u0 | u1 | u2 | u3;
			}

			And = v0 & v1 & v2 & v3;
			Or = v0 | v1 | v2 | v3;

			while ((And ^ Or) & 0x10000) {
				v0 += TileHeight;
				v1 += TileHeight;
				v2 += TileHeight;
				v3 += TileHeight;
				And = v0 & v1 & v2 & v3;
				Or = v0 | v1 | v2 | v3;
			}

			u0 = (u0 & 0xff00) >> 8;
			u1 = (u1 & 0xff00) >> 8;
			u2 = (u2 & 0xff00) >> 8;
			((ushort *)pFace)[8] = u0 | (v0 & 0xff00U);
			((ushort *)pFace)[10] = u1 | (v1 & 0xff00U);
			int u3Hi = u3 & 0xff00;
			int v3Hi = v3 & 0xff00;

			pFace[6] = u2 | (v2 & 0xff00) | (u3Hi << 8) | (v3Hi << 16);
			pFace = (uint *)((char *)pFace + (((ushort *)pFace)[1] & 0xfffc));
		}

		pData = (uint *)pTexWibFaceInfo;
	}
}

/* M3d_PreprocessPulsingColours__Fi (0x800856d4) */
/* Byte-identical demo twin; locals from its MAIN.SYM. Each pulse record is a header
   word {colour index, list length, list position, time} followed by a list of {r, g, b, interval}
   keys. It advances by the frame delta and interpolates the current colour with the GTE (GPL)
   into the region's colour table. */
void M3d_PreprocessPulsingColours(int Region) {
	uint *pData;
	uint *pEnd;
	uint dt;
	uint ListLen;
	uint ListPos;
	int t;
	int rgb0[3];
	int drgb[3];
	SRGBI *RGBs;
	int16_t k; /* the interpolation weight, 4.12 */

	if (Region == -1)
		return;

	pData = PSXRegion[Region].pColourPulseData;

	if (!pData)
		return;

	pEnd = (uint *)((char *)pData + pData[-1]);
	dt = XblanksNow - XblanksThen;
	pColourTable = PSXRegion[Region].pColourTable;

	if (XblanksNow < XblanksThen)
		dt = 1;

	while (pData < pEnd) {
		RGBs = (SRGBI *)(pData + 1);
		ListPos = ((uchar *)pData)[2];
		t = ((uchar *)pData)[3];
		t += dt;
		ListLen = ((uchar *)pData)[1];

		while (t >= RGBs[ListPos].Interval) {
			t -= RGBs[ListPos].Interval;

			if (++ListPos == ListLen)
				ListPos = 0;
		}

		((uchar *)pData)[2] = ListPos;
		((uchar *)pData)[3] = t;
		rgb0[0] = RGBs[ListPos].r << 4;
		rgb0[1] = RGBs[ListPos].g << 4;
		rgb0[2] = RGBs[ListPos].b << 4;

		{
			uint Old = ListPos; /* not in the demo SYM; it gives the copy of the key index */

			if (++ListPos == ListLen)
				ListPos = 0;

			drgb[0] = (RGBs[ListPos].r << 4) - rgb0[0];
			drgb[1] = (RGBs[ListPos].g << 4) - rgb0[1];
			drgb[2] = (RGBs[ListPos].b << 4) - rgb0[2];
			k = (int16_t)((t << 12) / RGBs[Old].Interval);
		}

		{
			/* rgb0 + drgb * k, saturated into a colour word (16-bit drgb and k) */
			int16_t d[3];
			uint32_t rgb;

			geom_ld_lv(d, drgb);
			geom_gpl(k, d, rgb0, 12, 0, 0, &rgb, 0);
			__builtin_memcpy(&pColourTable[((uchar *)pData)[0]], &rgb, 4);
		}

		pData = (uint *)((uchar *)pData + ListLen * 4) + 1;
	}
}
