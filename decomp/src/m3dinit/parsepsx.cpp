/* M3dInit_ParsePSX__Fi (0x800821f8) */
/* C++ (M3DINIT.CPP). Locals from the demo's MAIN.SYM; SModel/Texture are the demo's layouts.
   The per-region table (64-byte entries) is laid out by offset. */
#include "game/m3d.h"

/* one item record in a PSX file (SPSXRegion::pPSX + 3): the first 0x24 bytes of m3d.h's Item.
   game/m3d.h's Item runs on to 0x48 (CItem's data), so it can't step through the file's array */
struct SPSXItem {
	ushort Flags;
	ushort Inquiry;
	Vector Position;
	SVector Angles;
	ushort Model;
	uchar Frame;
	uchar TintIndex;
	uchar Anim;
	uchar Region;
	SPSXItem *NextItem;
	uint RGB;
};

extern SPSXRegion PSXRegion[];

int GHeadPart;
int GBoardPart;
int GWheel1Part;
int GWheel2Part;

extern const int GHeadUp;
extern ushort M3d_DefaultLODDistance;

void M3dInit_ParsePSX(int Region) {
	SModel *pModel;
	SPSXItem *pItem;
	uint *pFace;
	uint Fastable;
	int i;
	int j;
	int NumModels;
	int NumParts;
	int NumItems;
	int NumVertices;
	int NumNormals;
	int NumFaces;
	int NumStitchVertices;
	Texture *pTex;
	SVECTOR *pVertex;
	SVECTOR *pNormal;
	ushort clut;
	ushort tpage;
	ushort u0v0;
	ushort u1v1;
	ushort u2v2;
	ushort u3v3;
	uint OrOfFlags;
	uint AndOfFUFlags;

	if (GHeadPart != -1) {
		if (GHeadPart == 18) {
			GBoardPart = 0;
			GWheel1Part = 1;
			GWheel2Part = 2;
		} else {
			GBoardPart = 16;
			GWheel1Part = 17;
			GWheel2Part = 18;
		}
	}

#ifdef PC_PORT
	/* a Dreamcast (version 6) file keeps its faces' Texture pointers and 16-bit texture
	   coordinates; the renderer reads them (platform/pc/include/pc_dctex.h) */
	int DcFile = (PSXRegion[Region].pPSX[0] & 0xffff) == 6;
#endif

	NumStitchVertices = 0;
	Fastable = 1;
	NumParts = 0;
	NumModels = (int)PSXRegion[Region].ppModels[-1];
	pModel = PSXRegion[Region].ppModels[0];

	for (i = 0; i < NumModels; i++) {
		OrOfFlags = 0;
		NumVertices = pModel->NumVertices;
		NumNormals = pModel->NumNormals;
		NumFaces = pModel->NumFaces;
		AndOfFUFlags = 0xffffffff;

		if (pModel->NextLOD == 0xffff)
			NumParts++;

		if (pModel->zMax == -1)
			pModel->zMax = M3d_DefaultLODDistance;

		for (j = 0, pVertex = pModel->Vertices; j < NumVertices; j++, pVertex++) {
			if (i == GHeadPart && !(pVertex->pad & 0x12)) {
				pVertex->vx *= 3;
				pVertex->vy *= 2;
				pVertex->vz *= 3;
				pVertex->vy += GHeadUp * 16;

				if (GHeadUp < 0)
					pVertex->vz -= 240;
			}

			if (pVertex->pad & 2) {
				pVertex->vx = (ushort)pVertex->vy * 8;
				pVertex->vy = 0;
			}

			if (pVertex->pad & 0x10)
				pModel->Flags |= 0x100;

			if (pVertex->pad & 1)
				NumStitchVertices++;
		}

		if (NumVertices + NumStitchVertices > 100)
			Fastable = 0;

		if (NumNormals == NumFaces + NumVertices) {
			pNormal = pVertex;
			pVertex -= NumVertices;

			for (j = 0; j < NumVertices; j++, pNormal++, pVertex++) {
				if (pVertex->pad & 1)
					pNormal->pad |= 1;

				if (pVertex->pad & 2) {
					pNormal->pad |= 2;
					pNormal->vx = pVertex->vx;
					pNormal->vy = 0;
				}
			}
		}

		pFace = (uint *)(pVertex + NumNormals);

		for (j = 0; j < NumFaces; j++) {
			((ushort *)pFace)[6] *= 8;

			if (!(pFace[0] & 0x40))
				pFace[0] ^= 0x80;

			if (!(pFace[0] & 0x800))
				pFace[2] &= 0xffffff;

#ifdef PC_PORT
			if ((pFace[0] & 3) == 3 && !DcFile) {
#else
			if ((pFace[0] & 3) == 3) {
#endif
				pTex = (Texture *)pFace[4];
				clut = pTex->clut;
				tpage = pTex->tpage;
				u0v0 = ((ushort *)pFace)[10];
				u1v1 = ((ushort *)pFace)[11];
				u2v2 = ((ushort *)pFace)[12];
				u3v3 = ((ushort *)pFace)[13];

				if (pFace[0] & 0x20) {
					pFace[7] = pTex->TexWin;
				} else {
					/* not in the SYM: an int temp keeps the load a zero_extend (no HI-mode
					   load whose dying pTex would pull pTex into a0 by preference) */
					int uv = *(ushort *)pTex;

					u0v0 += uv;
					u1v1 += uv;
					u2v2 += uv;
					u3v3 += uv;
				}

				pFace[4] = u0v0 | (clut << 16);
				pFace[5] = u1v1 | (tpage << 16);
				pFace[6] = u2v2 | (u3v3 << 16);
			}

			OrOfFlags |= pFace[0];
			AndOfFUFlags &= ((ushort *)pFace)[7];

			if (pFace[0] & 0x10)
				((uchar *)pFace)[7] = ((uchar *)pFace)[6];

			pFace += pFace[0] >> 18;
		}

		if (AndOfFUFlags & 1)
			pModel->Flags |= 0x10;

		if (!(OrOfFlags & 0xc0))
			pModel->Flags |= 0x20;

		if (OrOfFlags & 0x1000)
			pModel->Flags |= 0x40;

		if (OrOfFlags & 4)
			pModel->Flags |= 4;

		pModel = (SModel *)pFace;
	}

	PSXRegion[Region].Fastable = Fastable;
	PSXRegion[Region].NumParts = NumParts;
	NumItems = PSXRegion[Region].pPSX[2];
	pItem = (SPSXItem *)(PSXRegion[Region].pPSX + 3);

	for (i = 0; i < NumItems; i++) {
		if (i < NumItems - 1)
			pItem->NextItem = pItem + 1;
		else
			pItem->NextItem = 0;

		pItem->Region = Region;

		if (PSXRegion[pItem->Region].ppModels[pItem->Model]->Flags & 0x10)
			pItem->Flags |= 0x20;

		if (PSXRegion[pItem->Region].ppModels[pItem->Model]->Flags & 0x20)
			pItem->Flags |= 0x1000;

		pItem++;
	}
}

extern const int GHeadUp = 0;
