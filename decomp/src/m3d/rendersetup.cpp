/* M3d_RenderSetup__FP7SCameraP9SViewportPUi (0x800831f8) flags=-fstructure-noalias */
/* C++ (M3D.CPP). Demo twin; locals from its MAIN.SYM (Window, pClipArea, opp, adj, hyp, pTex, i).
   Per-frame render setup: region-change count, camera/viewport/OT globals, the clip DR_AREA,
   viewport zoom and centre, the ten frustum plane normals (rotated into camera space with
   ApplyMatrixSV), geometry screen/offset, the scaled view matrix, the sky texture page and the
   pulsing/wibbly texture preprocessing. Levers: -fstructure-noalias; the four camera-up shorts
   at 0x800d2180 are one SVECTOR (a struct, so its stores stay ordered against the camera loads). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"
#include "game/camera.h"

int M3dMaths_SquareRoot0(int);
void M3dMaths_TransposeMat(ushort *, ushort *);
Texture *Spool_FindTextureEntry(uint);
void M3d_PreprocessPulsingColours(int);
void M3d_PreprocessWibblyTextures(int);

extern SVECTOR FrustumNormals[];
extern SVector FrustumMatrix1[];
extern SVector FrustumMatrix2[];
extern int Xblanks;
extern SDoubleBuffer *pDoubleBuffer;
extern DR_AREA *pPoly;
extern int EnvRegions;
extern int EnvRegions_4;
extern uchar CurrentObjFileRegion;
extern int CurrentWaterLevel;

int XblanksNow, XblanksThen, FrameNo;
ushort DpqMax, Xres, Yres;
SCamera *pCurrentCamera;
SViewport *pCurrentViewport;
uint *CurrentOrderingTable;
int PixelAspectX, PixelAspectY;
int RefMapClut, RefMapTpage, RefMapU0V0;
SVECTOR WaterNormal;

#ifdef PC_PORT
#include "pc_video.h"

/* Widescreen (platform/pc, pc_gpu_gl.h): a full-width view takes in pc_wide_pad more columns on
   either side at the same zoom (Hor+), so its side planes open up by as much. */
extern "C" int pc_wide_pad;

static int PC_ViewWidth(SViewport *pVP) {
	int w = pVP->xR - pVP->xL;

	if (pVP->xL == 0 && pVP->xR == Xres)
		w += 2 * pc_wide_pad;

	return w;
}
#endif

void M3d_RenderSetup(SCamera *pCamera, SViewport *pVP, uint *ot) {
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
#ifdef PC_PORT
	pVP->Yon = PC_DrawFarPlane(DpqMax); /* Options > Graphics > Draw Distance */
#else
	pVP->Yon = DpqMax;
#endif
	CurrentOrderingTable = ot;
	Window.x = 0;
	Window.y = *(ushort *)&pDoubleBuffer->Draw.clip.y;
	Window.w = Xres;
	Window.h = Yres;
	pClipArea = pPoly;
	SetDrawArea(pClipArea, &Window);

	pPoly = pClipArea + 1;
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

#ifdef PC_PORT
	opp = PC_ViewWidth(pVP) << 11;
#else
	opp = (pVP->xR - pVP->xL) << 11;
#endif
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

#ifdef PC_PORT
	opp = PC_ViewWidth(pVP) >> 1;
#else
	opp = (pVP->xR - pVP->xL) >> 1;
#endif
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
			ApplyMatrixSV(&pCamera->Transform, &FrustumNormals[i], (SVECTOR *)&FrustumMatrix1[i]);
			ApplyMatrixSV(&pCamera->Transform, &FrustumNormals[i + 3],
			              (SVECTOR *)&FrustumMatrix2[i]);
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

	pTex = Spool_FindTextureEntry(0xcce0187c);
	RefMapClut = pTex->clut << 16;
	RefMapTpage = pTex->tpage << 16;
	RefMapU0V0 = pTex->u0 | (pTex->v0 << 8);
	M3d_PreprocessPulsingColours(EnvRegions);
	M3d_PreprocessPulsingColours(EnvRegions_4);
	M3d_PreprocessPulsingColours(CurrentObjFileRegion);
	M3d_PreprocessWibblyTextures(CurrentObjFileRegion);

	WaterNormal.vx = -pCurrentCamera->Transform.m[1][0];
	WaterNormal.vy = -pCurrentCamera->Transform.m[1][1];
	WaterNormal.vz = -pCurrentCamera->Transform.m[1][2];
	WaterNormal.pad = (CurrentWaterLevel >> 12) - pCurrentCamera->Position.vy;
}
