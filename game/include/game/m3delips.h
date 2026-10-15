/* M3DELIPS.H: drawing a super item's part ellipsoids (textured) and its shadows (RenderSuperItem
   calls them per part) (M3DELIPS.CPP). */
#ifndef GAME_M3DELIPS_H
#define GAME_M3DELIPS_H

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"
#include "game/vector.h"

struct SModel;
struct Texture;

/* functions */
void M3dElips_RenderShadow(SModel *pModel, MATRIX *pTransform, CSVector *pNormal, CVector *pPoint, SVECTOR *pLightVec, CVECTOR rgb);
void M3dElips_RenderEllipsoid(SModel *pModel, MATRIX *pTransform, uint Rgb, int Scale, int OTPushback, Texture *pTex);

#endif
