/* M3d_PreprocessWibblyTextures__Fi (0x8008528c) */
/* C++ (M3D.CPP). THPS1's M3d_PreprocessWibblyTextures(Region): the demo twin minus its
   ZeroU/ZeroVAmplitudes skips and the 16-pass cap on the tile-wrap loops, with u/v wibble calls
   interleaved per vertex. vWibble__FP15STexWibVertInfo is vWibble. Locals and line order from the demo SYM.
   Matching notes:
   - The uv3 half of the last word goes through masked block locals (u3Hi/v3Hi) so local-alloc puts
     them in a0/a1 and the or-chain in v0, which lets sched2 hoist them as retail does.
   - The do{}while(0) wrappers around the v2/v3 wibble adds raise their global-alloc refs (40 -> 42)
     to give retail's u1,u0,u2,v3,v2,u3,v1,v0 priority order (s1..fp).
   - The `0xff00U` masks matter: an unsigned mask stops the store to a ushort narrowing the AND
     into `li -256`. */
#include "game/m3d.h"
#include "game/psyq.h"

extern SPSXRegion PSXRegion[];

int XblanksNow;
int BasePhase;

int uWibble(STexWibVertInfo *pTexWibVertInfo);
int vWibble(STexWibVertInfo *pTexWibVertInfo);

/* Dreamcast (version 6) files, platform/pc/include/pc_dctex.h: the face info's u and v are 0, and
   the scroll and wibble move the face's own 16-bit coordinates (u0..u3, v0..v3 in words 5-8), so
   those are kept from the first call after the file loaded. The offsets are in PS1 texels (8.8);
   a PS1 texel is taken as 8 units (512 = one repeat of a texture 64 texels wide, the usual
   size). */
#include <stdlib.h>
#include <string.h>

struct SDcWibBase {
	uint *pData;
	uint *pPSX;
	int NumFaces;
	ushort *pUV;
};

static SDcWibBase DcWibBase[64];

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
			Item *pItem = (Item *)((char *)pPSX + *p);
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
		Item *pItem = (Item *)((char *)pPSX + *p);
		STexWibFaceInfo *pInfo = (STexWibFaceInfo *)(p + 4);
		int NumFaces = p[3];
		int i;

		if (pItem->Flags & 0x8000) {
			n += NumFaces;
			continue;
		}

		SModel *pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
		uint *pFace = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
		int uScroll = (XblanksNow * ((short *)p)[2]) >> 4;
		int vScroll = (XblanksNow * ((short *)p)[3]) >> 4;

		BasePhase = (XblanksNow * (int)p[2]) >> 10;

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
		pItem = (Item *)((char *)pPSX + *pData);
		pTexWibFaceInfo = (STexWibFaceInfo *)(pData + 4);
		NumFaces = pData[3];

		if (pItem->Flags & 0x8000) {
			pData = (uint *)(pTexWibFaceInfo + NumFaces);
			continue;
		}

		pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
		pFace = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
		uScroll = (XblanksNow * ((short *)pData)[2]) >> 4;
		vScroll = (XblanksNow * ((short *)pData)[3]) >> 4;
		BasePhase = (XblanksNow * (int)pData[2]) >> 10;

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
