/* Shatter_Face__FP4ItemPUiiiiii (0x800218a4) */
/* C++ (SHATTER.CPP). Fuzzy demo twin (0.98); locals from its MAIN.SYM. Shatters one face of an
   item's model. Optionally it first checks/updates the face's shatterable flags. Then it
   transforms the face's three or four vertices into world space and sets the colour and centroid.
   Untextured (or flagged) faces become a flash; textured ones are split into shards (a quad as
   two triangles). Returns 0 if not shatterable, 1 if nothing was tried, 2 for a flash. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
}

#include "game/m3d.h"
#include "game/m3dasm.h"

static inline void SetV(CVector *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

extern SPSXRegion PSXRegion[];
extern CVector DAT_800d2574;
extern int LowMemory;

int VRAMClut;
int VRAMTPage;
SVECTOR *pNormal;
uchar AverageR;
uchar AverageG;
uchar AverageB;

void TransformVertex(CVector &, SVECTOR *, uchar *, int);
void CalcRGB(int, uint, int, uint *);
void Shatter_Glass(int, const CVector &, const CVector &, const CVector &, const CVector &, uchar,
                   uchar, uchar);
void Split(const CVector &, const CVector &, const CVector &, int, int, int, int, int, int, uint,
           int);

int Shatter_Face(Item *pItem, uint *pFace, int NumSplits, int MakeFlash, int UseFlags,
                 int ModifyFlags, int TryToShatter) {
	SModel *pModel;
	SVECTOR *pVertices;
	SVECTOR *pNormals;
	uchar *pVertexNums;
	Texture *pTexture;
	MATRIX Transform;
	CVector a;
	CVector b;
	CVector c;
	int retval;
	SFace *f = (SFace *)pFace;

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
	pVertices = pModel->Vertices;
	pNormals = pVertices + pModel->NumVertices;
	pVertexNums = (uchar *)(pFace + 1);

	if (UseFlags) {
		if (!(f->Flags2 & 8))
			return 0;

		if (ModifyFlags) {
			f->Flags2 = (f->Flags2 | 1) & ~8;
			f->Flags &= ~0x80;
			f->Flags &= ~0x100;

			if (!TryToShatter && ((f->Flags & 0x40) || !(f->Flags & 1)))
				f->Flags &= ~0x140;
		}
	}

	if (!TryToShatter)
		return 1;

	if (f->Flags & 1) {
#ifdef PC_PORT
		/* a Dreamcast face keeps its Texture pointer (pc_dctex.h); the shards use the whole
		   texture */
		if ((f->Flags & 2) && (uint)f->pTexture < 0x80000000)
#else
		if (f->Flags & 2)
#endif
			pTexture = (Texture *)&f->pTexture;
		else
			pTexture = f->pTexture;

		{
			int clut = pTexture->clut;
			int tpage = pTexture->tpage;

			VRAMClut = clut;
			VRAMTPage = tpage;
		}
	}

	pNormal = (SVECTOR *)((char *)pNormals + (f->NormalOffset & ~7));
	M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &Transform);
	SetRotMatrix(&Transform);
	M3dAsm_SetTransVector((Vector *)&pItem->Position);
	TransformVertex(a, pVertices, pVertexNums, 0);
	TransformVertex(b, pVertices, pVertexNums, 1);
	TransformVertex(c, pVertices, pVertexNums, 2);

	retval = 1;

	if (f->Flags & 16) {
		CalcRGB(f->Flags & 0x800, f->Colour, 3, PSXRegion[pItem->Region].pColourTable);

		DAT_800d2574.vx = ((a.vx >> 12) + (b.vx >> 12) + (c.vx >> 12)) / 3 << 12;
		DAT_800d2574.vy = ((a.vy >> 12) + (b.vy >> 12) + (c.vy >> 12)) / 3 << 12;
		DAT_800d2574.vz = ((a.vz >> 12) + (b.vz >> 12) + (c.vz >> 12)) / 3 << 12;

		if ((f->Flags & 0x40) || !(f->Flags & 1)) {
			if (!LowMemory) {
				CVector norm;

				SetV(&norm, pNormal->vx, pNormal->vy, pNormal->vz);
				Shatter_Glass(15, a, b, c, norm, AverageR, AverageG, AverageB);

				retval = 2;
			}

			if (ModifyFlags)
				f->Flags &= ~0x40;
		} else {
			uint TextureWindow;

			if (f->Flags & 32)
				TextureWindow = pTexture->TexWin;
			else
				TextureWindow = 0;

			if (!LowMemory)
				Split(a, b, c, pTexture->u0, pTexture->v0, pTexture->u1, pTexture->v1, pTexture->u2,
				      pTexture->v2, TextureWindow, NumSplits);
		}
	} else {
		CVector d;

		TransformVertex(d, pVertices, pVertexNums, 3);
		CalcRGB(f->Flags & 0x800, f->Colour, 4, PSXRegion[pItem->Region].pColourTable);

		DAT_800d2574.vx = ((a.vx >> 12) + (b.vx >> 12) + (c.vx >> 12) + (d.vx >> 12)) >> 2 << 12;
		DAT_800d2574.vy = ((a.vy >> 12) + (b.vy >> 12) + (c.vy >> 12) + (d.vy >> 12)) >> 2 << 12;
		DAT_800d2574.vz = ((a.vz >> 12) + (b.vz >> 12) + (c.vz >> 12) + (d.vz >> 12)) >> 2 << 12;

		if ((f->Flags & 0x40) || !(f->Flags & 1)) {
			if (!LowMemory) {
				CVector norm;

				SetV(&norm, pNormal->vx, pNormal->vy, pNormal->vz);
				Shatter_Glass(30, a, b, c, norm, AverageR, AverageB, AverageB);

				retval = 2;
			}

			if (ModifyFlags)
				f->Flags &= ~0x40;
		} else {
			uint TextureWindow;

			if (f->Flags & 32)
				TextureWindow = pTexture->TexWin;
			else
				TextureWindow = 0;

			if (!LowMemory) {
				Split(a, b, c, pTexture->u0, pTexture->v0, pTexture->u1, pTexture->v1, pTexture->u2,
				      pTexture->v2, TextureWindow, NumSplits);
				Split(b, c, d, pTexture->u1, pTexture->v1, pTexture->u2, pTexture->v2, pTexture->u3,
				      pTexture->v3, TextureWindow, NumSplits);
			}
		}
	}

	return retval;
}
