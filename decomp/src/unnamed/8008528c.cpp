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

extern SPSXRegion PSXRegion[];

int XblanksNow;
int BasePhase;

int uWibble(STexWibVertInfo *pTexWibVertInfo);
int vWibble(STexWibVertInfo *pTexWibVertInfo);

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
