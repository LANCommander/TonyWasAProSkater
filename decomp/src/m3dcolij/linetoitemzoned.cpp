/* M3dColij_LineToItemZoned__FPP4ItemP9SLineInfo (0x80080328) flags=-fstructure-noalias */
/* C++ (M3DCOLIJ.CPP). Demo twin; locals from its MAIN.SYM (pItem, i, pModel, Faces, ItemX/Y/Z,
   NumFaces, pVertices, pFaceX, pFaceCache, pVertexIndex, z0, x/y/zmin, x/y/zmax, x1..z3).
   Tests the line against a zero-terminated item list. Each item is visited once per inquiry; an
   item already in the 10-entry collision cache is tested from its cached face boxes, otherwise it
   replaces the next cache slot: the model's per-face world bounding boxes are built into the
   face cache (500 faces, wrapping), cache usage stats are updated, and the item is tested.
   Levers: -fstructure-noalias; a block-local face-loop `int i` (s6) separate from the cache i;
   the high-water test written `used > max`. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/colij.h"

extern SPSXRegion PSXRegion[];
extern SColCache ItemCache[];
extern SFaceCache FaceCache[];

int ItemCacheIndex;
int DAT_800d20e4;
int DAT_800d20e8;
int DAT_800d2110;
int DAT_800d2114;
int DAT_800d2118;
int DAT_800d211c;

void M3dAsm_LineColijPreprocessItemsZoned(Item **, int, SLineInfo *, ushort);
void M3dColij_LineToThisItemCached(Item *, SLineInfo *, SColCache *);

void M3dColij_LineToItemZoned(Item **ppItem, SLineInfo *pInfo) {
	if (!ppItem)
		return;

	if (!pInfo->Length)
		return;

	gte_SetRotMatrix(&pInfo->WorldCst);
	M3dAsm_LineColijPreprocessItemsZoned(ppItem, 0, pInfo, pInfo->Inquiry);

	while (*ppItem) {
		if ((*ppItem)->Inquiry != pInfo->Inquiry) {
			Item *pItem;
			int i;

			(*ppItem)->Inquiry = pInfo->Inquiry;
			pItem = *ppItem;

			for (i = 0; i < 10; i++)
				if (ItemCache[i].pItem == pItem)
					break;

			if (i != 10) {
				M3dColij_LineToThisItemCached(pItem, pInfo, &ItemCache[i]);

				DAT_800d2110++;
			} else {
				SModel *pModel;
				int Faces;
				int ItemX;
				int ItemY;
				int ItemZ;
				int NumFaces;
				SVECTOR *pVertices;
				uint *pFaceX;
				SFaceCache *pFaceCache;

				if (ItemCache[ItemCacheIndex].pItem) {
					if (ItemCacheIndex + 1 == 10)
						DAT_800d20e8 = ItemCache[0].FirstFace;
					else
						DAT_800d20e8 = ItemCache[ItemCacheIndex + 1].FirstFace;
				}

				ItemCache[ItemCacheIndex].pItem = pItem;
				pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
				ItemCache[ItemCacheIndex].pModel = pModel;
				Faces = pModel->NumFaces;
				ItemCache[ItemCacheIndex].Faces = Faces;

				if (DAT_800d20e4 + Faces < 500) {
					ItemCache[ItemCacheIndex].FirstFace = DAT_800d20e4;
					DAT_800d20e4 += Faces;
				} else {
					ItemCache[ItemCacheIndex].FirstFace = 0;
					DAT_800d20e4 = Faces;
				}

				pVertices = pModel->Vertices;
				pFaceX = (uint *)(&pModel->Vertices[pModel->NumVertices] + pModel->NumNormals);
				pFaceCache = &FaceCache[ItemCache[ItemCacheIndex].FirstFace];
				ItemX = pItem->Position.vx;
				ItemY = pItem->Position.vy;
				ItemZ = pItem->Position.vz;
				NumFaces = pModel->NumFaces;

				for (int i = 0; i < NumFaces; i++) {
					uchar *pVertexIndex;
					int z0;
					int xmin, ymin, zmin;
					int xmax, ymax, zmax;
					int x1, y1, z1;
					int x2, y2, z2;
					int x3, y3, z3;

					pFaceCache->Flags = *(ushort *)pFaceX;
					pVertexIndex = (uchar *)(pFaceX + 1);
					z0 = pVertices[pVertexIndex[0]].vz;
					xmin = pVertices[pVertexIndex[0]].vx;
					ymin = pVertices[pVertexIndex[0]].vy;
					zmin = z0;
					xmax = xmin;
					ymax = ymin;
					zmax = zmin;
					x1 = pVertices[pVertexIndex[1]].vx;
					y1 = pVertices[pVertexIndex[1]].vy;
					z1 = pVertices[pVertexIndex[1]].vz;
					x2 = pVertices[pVertexIndex[2]].vx;
					y2 = pVertices[pVertexIndex[2]].vy;
					z2 = pVertices[pVertexIndex[2]].vz;

					if (xmax < x1)
						xmax = x1;

					if (xmax < x2)
						xmax = x2;

					if (ymax < y1)
						ymax = y1;

					if (ymax < y2)
						ymax = y2;

					if (zmax < z1)
						zmax = z1;

					if (zmax < z2)
						zmax = z2;

					if (x1 < xmin)
						xmin = x1;

					if (x2 < xmin)
						xmin = x2;

					if (y1 < ymin)
						ymin = y1;

					if (y2 < ymin)
						ymin = y2;

					if (z1 < zmin)
						zmin = z1;

					if (z2 < zmin)
						zmin = z2;

					x3 = pVertices[pVertexIndex[3]].vx;
					y3 = pVertices[pVertexIndex[3]].vy;
					z3 = pVertices[pVertexIndex[3]].vz;

					if (!(*(ushort *)pFaceX & 16)) {
						if (xmax < x3)
							xmax = x3;

						if (ymax < y3)
							ymax = y3;

						if (zmax < z3)
							zmax = z3;

						if (x3 < xmin)
							xmin = x3;

						if (y3 < ymin)
							ymin = y3;

						if (z3 < zmin)
							zmin = z3;
					}

					xmin <<= 12;
					ymin <<= 12;
					zmin <<= 12;
					xmax <<= 12;
					ymax <<= 12;
					zmax <<= 12;
					xmin += ItemX;
					ymin += ItemY;
					zmin += ItemZ;
					xmax += ItemX;
					ymax += ItemY;
					zmax += ItemZ;
					pFaceCache->XMin = xmin;
					pFaceCache->YMin = ymin;
					pFaceCache->ZMin = zmin;
					pFaceCache->XMax = xmax;
					pFaceCache->YMax = ymax;
					pFaceCache->ZMax = zmax;
					pFaceCache++;
					pFaceX = (uint *)((char *)pFaceX + (((ushort *)pFaceX)[1] & ~3));
				}

				DAT_800d2118 = 0;

				for (i = 0; i < 10; i++)
					if (ItemCache[i].pItem)
						DAT_800d2118 += ItemCache[i].Faces;

				if (DAT_800d2118 > DAT_800d211c)
					DAT_800d211c = DAT_800d2118;

				M3dColij_LineToThisItemCached(pItem, pInfo, &ItemCache[ItemCacheIndex]);

				if (++ItemCacheIndex == 10)
					ItemCacheIndex = 0;

				DAT_800d2114++;
			}
		}

		ppItem++;
	}
}
