/* M3DCOLIJ.CPP: line collision against the level's items: the line query setup, the item and
   face caches, per-item tests (with trigger faces firing their nodes) and line-to-sphere tests
   against bodies (declarations in game/m3dcolij.h; its structs in game/colij.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include "game/scratchpad.h"
}

#include "game/m3dcolij.h"
#include "game/colij.h"
#include "game/m3dasm.h"
#include "game/trig.h"
#include "game/vector.h"
#include "game/ob.h"
#include "game/globals.h"
#include "game/spool.h"

extern ushort Inquiry;
extern MATRIX Tfm;
extern int ItemCacheIndex;
extern int DAT_800d20e4;
extern int DAT_800d20e8;
extern SColCache ItemCache[10];
extern SFaceCache FaceCache[];
extern SHit NearestHit;
extern int DAT_800d2110;
extern int DAT_800d2114;
extern int DAT_800d2118;
extern int DAT_800d211c;
extern int CollidedTriggerModel;
extern CBody *pBodyHit;
void NextInquiry();
extern "C" {
	int Mips_DotProd2(int, int, int, int);
}
void M3dAsm_LineColijPreprocessItems(Item *, int, SLineInfo *, ushort);
void M3dAsm_LineColijPreprocessItemsZoned(Item **, int, SLineInfo *, ushort);
void M3dColij_LineToThisItem(Item *, SLineInfo *);
void M3dColij_LineToThisItemCached(Item *, SLineInfo *, SColCache *);

/* NextInquiry__Fv (0x8007ea94) */
void NextInquiry(void) {
	Item *p;

	if (++Inquiry == 0) {
		Inquiry = 1;

		for (p = EnviroList; p; p = p->NextItem)
			p->Inquiry = 0;

		for (p = (Item *)EnvironmentalObjectList; p; p = p->NextItem)
			p->Inquiry = 0;
	}
}

/* M3dColij_InitLineInfo__FP9SLineInfo (0x8007eb08) */
/* Locals, types and scopes from the demo's MAIN.SYM, except the two short
   cosines, which the SYM doesn't list. Cos2 has to be a fresh variable: reassigning r1Norm gives it
   two deaths, so local-alloc skips it and it loses s1 to Shift1. */
void M3dColij_InitLineInfo(SLineInfo *pInfo) {
	short *pShort;
	SVECTOR L;
	SVECTOR AbsL;
	Vector *p1;
	int lzc;
	int Shift1;
	int Shift2;
	int s1;
	int s2;
	int r1Norm;
	int r2Norm;
	short Sin;

	pInfo->pItem = 0;
	pInfo->pFace = 0;
	pInfo->Distance = 0x7fffffff;
	pInfo->tNear = 0x7fffffff;
	pInfo->Model = -1;
	pInfo->RecordTriggerZoneHits = 0;
	pInfo->DropDown = 0;

	L.vx = (pInfo->EndCoords.vx - pInfo->StartCoords.vx) >> 12;
	L.vy = (pInfo->EndCoords.vy - pInfo->StartCoords.vy) >> 12;
	L.vz = (pInfo->EndCoords.vz - pInfo->StartCoords.vz) >> 12;

	s1 = L.vx * L.vx + L.vz * L.vz;

	if (s1) {
		lzc = geom_lzc(s1); /* leading zeros */

		Shift1 = (lzc - 1) >> 1;
		r1Norm = M3dMaths_SquareRoot0(s1 << ((lzc - 1) & 0x1e));
		Sin = (L.vx << (Shift1 + 12)) / r1Norm;
		short Cos = (L.vz << (Shift1 + 12)) / r1Norm;

		pShort = (short *)&Tfm;
		*pShort++ = Cos;
		*pShort++ = 0;
		*pShort++ = -Sin;
		*pShort++ = 0;
		*pShort++ = 4096;
		*pShort++ = 0;
		*pShort++ = Sin;
		*pShort++ = 0;
		*pShort = Cos;

		s2 = s1 + L.vy * L.vy;
		lzc = geom_lzc(s2);

		Shift2 = (lzc - 1) >> 1;
		r2Norm = M3dMaths_SquareRoot0(s2 << ((lzc - 1) & 0x1e));
		pInfo->Length = r2Norm >> Shift2;
		Sin = (L.vy << (Shift2 + 12)) / r2Norm;
		short Cos2 = (r1Norm << 12) / (r2Norm << (Shift1 - Shift2));

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = Cos2;
		pInfo->WorldCst.m[1][2] = -Sin;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = Sin;
		pInfo->WorldCst.m[2][2] = Cos2;
		MulMatrix(&pInfo->WorldCst, &Tfm);
	} else {
		Sin = 4096;

		if (L.vy < 0) {
			Sin = -4096;
			pInfo->Length = -L.vy;
		} else {
			pInfo->Length = L.vy;
			pInfo->DropDown = 1;
		}

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = 0;
		pInfo->WorldCst.m[1][2] = -Sin;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = Sin;
		pInfo->WorldCst.m[2][2] = 0;
	}

	geom_ld_mat(pc_geom.rot, &pInfo->WorldCst);

	p1 = &pInfo->EndCoords;

	if (pInfo->StartCoords.vx < p1->vx) {
		pInfo->MinCoords.vx = pInfo->StartCoords.vx;
		pInfo->MaxCoords.vx = p1->vx;
	} else {
		pInfo->MinCoords.vx = p1->vx;
		pInfo->MaxCoords.vx = pInfo->StartCoords.vx;
	}

	if (pInfo->StartCoords.vy < p1->vy) {
		pInfo->MinCoords.vy = pInfo->StartCoords.vy;
		pInfo->MaxCoords.vy = p1->vy;
	} else {
		pInfo->MinCoords.vy = p1->vy;
		pInfo->MaxCoords.vy = pInfo->StartCoords.vy;
	}

	if (pInfo->StartCoords.vz < p1->vz) {
		pInfo->MinCoords.vz = pInfo->StartCoords.vz;
		pInfo->MaxCoords.vz = p1->vz;
	} else {
		pInfo->MinCoords.vz = p1->vz;
		pInfo->MaxCoords.vz = pInfo->StartCoords.vz;
	}

	NextInquiry();

	pInfo->Inquiry = Inquiry;
}

/* M3dColij_LineToItem__FP4ItemP9SLineInfo (0x8007ee84) */
void M3dColij_LineToItem(Item *it, SLineInfo *li) {
	if (it && li->Length) {
		geom_ld_mat(pc_geom.rot, &li->WorldCst);
		M3dAsm_LineColijPreprocessItems(it, 0, li, li->Inquiry);

		do {
			if (it->Inquiry != li->Inquiry) {
				it->Inquiry = li->Inquiry;
				M3dColij_LineToThisItem(it, li);
			}

			it = it->NextItem;
		} while (it);
	}
}

/* FUN_8007ef30 (0x8007ef30) */
extern "C" void FUN_8007ef30() {
	int i;

	ItemCacheIndex = 0;
	DAT_800d20e4 = 0;
	DAT_800d20e8 = -1;

	for (i = 0; i < 10; i++)
		ItemCache[i].pItem = 0;
}

static inline void FireNode(int node, int env) {
	OtherEnvIndex = env ^ 1;
	TriggerCollisionCheck = 0;
	Trig_TriggerCommandPoint(node, 1);
}

/* M3dColij_LineToThisItemCached__FP4ItemP9SLineInfoP9SColCache (0x8007ef68) */
/* Fuzzy demo twin (0.52); locals from its MAIN.SYM. Tests the line against
   an item whose faces are in the collision face cache. Each cached face box is checked against
   the line's box (grown by 4 units), then the face against the collide masks; the line's end
   points (item-relative) are tested against the face plane, then against the triangle or quad
   edges with 64-bit triple products. A hit on a trigger face records the model for its node,
   otherwise the nearest hit's position, face and distance are kept in pInfo.
   THPS1 differs from the demo in the tail (hit point via Mips_DotProd2) and drops the
   both-ends-on-plane reject.
   Levers: the face-cache walk is `pFaceCache->` with `pFaceCache++` at the loop end (two
   reduced givs, +12/+24); a `cmp != 1` stored in an int (xori); pItem->Pos read into the Q
   temps before any pInfo store; NearestHit_8 as an unsized extern; NearestHit reached through
   a pointer set in the loop guard before a do-while (it is spilled and rematerialised as
   `la s5` + 4(s5), and its high-part temp in the preheader pushes pFaceCache from v0 to v1). */
void M3dColij_LineToThisItemCached(Item *pItem, SLineInfo *pInfo, SColCache *pColCache) {
	int PosnRelStart[4];
	SModel *pModel;
	int XMin;
	int YMin;
	int ZMin;
	int XMax;
	int YMax;
	int ZMax;
	SFaceCache *pFaceCache;
	int Faces;
	int NumNormals;
	SVECTOR *pVerts;
	SVECTOR *Normals;
	uint *pFace;
	int tNear;
	int CollidedTriggerModel;
	SHit *pColij;

	XMin = pInfo->MinCoords.vx;
	YMin = pInfo->MinCoords.vy;
	ZMin = pInfo->MinCoords.vz;
	XMax = pInfo->MaxCoords.vx;
	YMax = pInfo->MaxCoords.vy;
	ZMax = pInfo->MaxCoords.vz;
	XMin = ((XMin >> 12) << 12) - 0x4000;
	YMin = ((YMin >> 12) << 12) - 0x4000;
	ZMin = ((ZMin >> 12) << 12) - 0x4000;
	XMax = ((XMax >> 12) << 12) + 0x4000;
	YMax = ((YMax >> 12) << 12) + 0x4000;
	ZMax = ((ZMax >> 12) << 12) + 0x4000;
	pFaceCache = &FaceCache[pColCache->FirstFace];
	Faces = pColCache->Faces;
	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
	NumNormals = pModel->NumNormals;
	pVerts = pModel->Vertices;
	Normals = &pModel->Vertices[pModel->NumVertices];
	pFace = (uint *)(Normals + NumNormals);
	tNear = pInfo->tNear;
	CollidedTriggerModel = -1;

	{
		int i;

		i = 0;

		if (Faces > 0) {
			pColij = &NearestHit;

			do {
				if (YMax >= pFaceCache->YMin && pFaceCache->YMax >= YMin &&
				    XMax >= pFaceCache->XMin && pFaceCache->XMax >= XMin &&
				    ZMax >= pFaceCache->ZMin && pFaceCache->ZMax >= ZMin) {
					ushort FaceFlags;
					uchar *pVertexIndex;
					int Nx, Ny, Nz;
					int x0, y0, z0;
					int L0x, L0y, L0z;
					int L1x, L1y, L1z;
					int LineLen;
					int s0, s1;
					int x1, y1, z1;
					int x2, y2, z2;
					int x3, y3, z3;

					long long Dx, Dy, Dz;
					int V0x, V0y, V0z;
					int V1x, V1y, V1z;
					int V2x, V2y, V2z;
					int V3x, V3y, V3z;

					long long S0, S1, S2, S3;
					int contained;

					if (pFace[3] & M3dColij_OneMask)
						goto next;

					if (~(pFace[3] | M3dColij_ZeroMask))
						goto next;

					{
						int ok = ((pFace[3] >> 16) & 3) != 1;

						if (!ok)
							goto next;
					}

					FaceFlags = *(ushort *)pFace;
					pVertexIndex = (uchar *)(pFace + 1);
					Nx = Normals[((ushort *)pFace)[6] >> 3].vx;
					Ny = Normals[((ushort *)pFace)[6] >> 3].vy;
					Nz = Normals[((ushort *)pFace)[6] >> 3].vz;
					x0 = pVerts[pVertexIndex[0]].vx;
					y0 = pVerts[pVertexIndex[0]].vy;
					z0 = pVerts[pVertexIndex[0]].vz;
					L0x = (pInfo->StartCoords.vx >> 12) - (pItem->Position.vx >> 12);
					L0y = (pInfo->StartCoords.vy >> 12) - (pItem->Position.vy >> 12);
					L0z = (pInfo->StartCoords.vz >> 12) - (pItem->Position.vz >> 12);
					L1x = (pInfo->EndCoords.vx >> 12) - (pItem->Position.vx >> 12);
					L1y = (pInfo->EndCoords.vy >> 12) - (pItem->Position.vy >> 12);
					L1z = (pInfo->EndCoords.vz >> 12) - (pItem->Position.vz >> 12);
					LineLen = pInfo->Length;
					s0 = Nx * (L0x - x0) + Ny * (L0y - y0) + Nz * (L0z - z0);
					s1 = Nx * (L1x - x0) + Ny * (L1y - y0) + Nz * (L1z - z0);

					if (s0 < 0 || s1 > 0)
						goto next;

					x1 = pVerts[pVertexIndex[1]].vx;
					y1 = pVerts[pVertexIndex[1]].vy;
					z1 = pVerts[pVertexIndex[1]].vz;
					x2 = pVerts[pVertexIndex[2]].vx;
					y2 = pVerts[pVertexIndex[2]].vy;
					z2 = pVerts[pVertexIndex[2]].vz;

					if (!(FaceFlags & 16)) {
						x3 = pVerts[pVertexIndex[3]].vx;
						y3 = pVerts[pVertexIndex[3]].vy;
						z3 = pVerts[pVertexIndex[3]].vz;
					}

					Dx = L1x - L0x;
					Dy = L1y - L0y;
					Dz = L1z - L0z;
					V0x = x0 - L0x;
					V0y = y0 - L0y;
					V0z = z0 - L0z;
					V1x = x1 - L0x;
					V1y = y1 - L0y;
					V1z = z1 - L0z;
					V2x = x2 - L0x;
					V2y = y2 - L0y;
					V2z = z2 - L0z;
					V3x = x3 - L0x;
					V3y = y3 - L0y;
					V3z = z3 - L0z;
					contained = 0;

					if (FaceFlags & 16) {
						S0 = Dx * (V2y * V0z - V2z * V0y) + Dy * (V2z * V0x - V2x * V0z) +
						     Dz * (V2x * V0y - V2y * V0x);
						S1 = Dx * (V0y * V1z - V0z * V1y) + Dy * (V0z * V1x - V0x * V1z) +
						     Dz * (V0x * V1y - V0y * V1x);
						S2 = Dx * (V1y * V2z - V1z * V2y) + Dy * (V1z * V2x - V1x * V2z) +
						     Dz * (V1x * V2y - V1y * V2x);

						if (S0 >= 0 && S1 >= 0 && S2 >= 0)
							contained = 1;
					} else {
						S0 = Dx * (V2y * V0z - V2z * V0y) + Dy * (V2z * V0x - V2x * V0z) +
						     Dz * (V2x * V0y - V2y * V0x);
						S1 = Dx * (V0y * V1z - V0z * V1y) + Dy * (V0z * V1x - V0x * V1z) +
						     Dz * (V0x * V1y - V0y * V1x);
						S2 = Dx * (V1y * V3z - V1z * V3y) + Dy * (V1z * V3x - V1x * V3z) +
						     Dz * (V1x * V3y - V1y * V3x);
						S3 = Dx * (V3y * V2z - V3z * V2y) + Dy * (V3z * V2x - V3x * V2z) +
						     Dz * (V3x * V2y - V3y * V2x);

						if (S0 >= 0 && S1 >= 0 && S2 >= 0 && S3 >= 0)
							contained = 1;
					}

					if (contained) {
						pInfo->Model = pItem->Model;

						if (((ushort *)pFace)[7] & 2) {
							if (pInfo->RecordTriggerZoneHits)
								CollidedTriggerModel = pItem->Model;
						} else {
							int Qx, Qy, Qz;
							int t;

							t = M3dMaths_MulDiv64(s0, 0x4000, s0 - s1);

							if (t < tNear) {
								Qx = Mips_DotProd2(L0x << 12, 0x4000, (int)Dx << 12, t) >> 2;
								Qy = Mips_DotProd2(L0y << 12, 0x4000, (int)Dy << 12, t) >> 2;
								Qz = Mips_DotProd2(L0z << 12, 0x4000, (int)Dz << 12, t) >> 2;
								Qx += pItem->Position.vx;
								Qy += pItem->Position.vy;
								Qz += pItem->Position.vz;
								tNear = t;
								pInfo->tNear = t;
								pInfo->Position.vx = Qx;
								pInfo->Position.vy = Qy;
								pInfo->Position.vz = Qz;
								pInfo->pItem = pItem;
								pInfo->pFace = pFace;
								pInfo->Model = pItem->Model;
								pInfo->Distance = Mips_FixMul(LineLen, tNear) >> 2;
								NearestHit.pNormal =
								    (SVECTOR *)((char *)Normals + (((ushort *)pFace)[6] & 0xfff8));
								pColij->pFace = pFace;
							}
						}
					}
				}
			next:
				pFace = (uint *)((char *)pFace + (((ushort *)pFace)[1] & ~3));
				i++;
				pFaceCache++;
			} while (i < Faces);
		}
	}

	if (CollidedTriggerModel != -1) {
		int i = Spool_GetEnvIndex(pItem->Region);

		FireNode(EnvModelChecksums[i][CollidedTriggerModel], i);
	}
}

/* M3dColij_LineToItemZoned__FPP4ItemP9SLineInfo (0x80080328) */
/* Demo twin; locals from its MAIN.SYM (pItem, i, pModel, Faces, ItemX/Y/Z,
   NumFaces, pVertices, pFaceX, pFaceCache, pVertexIndex, z0, x/y/zmin, x/y/zmax, x1..z3).
   Tests the line against a zero-terminated item list. Each item is visited once per inquiry; an
   item already in the 10-entry collision cache is tested from its cached face boxes, otherwise it
   replaces the next cache slot: the model's per-face world bounding boxes are built into the
   face cache (500 faces, wrapping), cache usage stats are updated, and the item is tested.
   Levers: -fstructure-noalias; a block-local face-loop `int i` (s6) separate from the cache i;
   the high-water test written `used > max`. */
void M3dColij_LineToItemZoned(Item **ppItem, SLineInfo *pInfo) {
	if (!ppItem)
		return;

	if (!pInfo->Length)
		return;

	geom_ld_mat(pc_geom.rot, &pInfo->WorldCst);
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

/* M3dColij_GetLineInfo__FP9SLineInfo (0x800808b0) */
int M3dColij_GetLineInfo(SLineInfo *li) {
	if (li->pItem) {
		M3dMaths_RotMatrixYXZ((SVECTOR *)&li->pItem->Angles, &Tfm);
		geom_ld_mat(pc_geom.rot, &Tfm);
		{
			int16_t v[3];
			GeomAcc r;

			geom_ld_sv(v, NearestHit.pNormal);
			r = geom_mvmva(pc_geom.rot, 0, v[0], v[1], v[2], 12, 0, 0);
			geom_st_sv(&li->Normal, r.ir);
		}
		return 1;
	}

	return 0;
}

/* M3dColij_LineToThisItem__FP4ItemP9SLineInfo (0x80080964) */
/* Fuzzy demo twin (0.97); locals from its MAIN.SYM. Tests the line against one
   item's model. An unrotated, unscaled item just offsets the line start. Otherwise the item's
   matrix is built in Tfm, with the axis-swapped variant for flagged lines. The model's
   vertices are projected (into the scratchpad when small enough) and the faces tested. A hit on a
   trigger face fires that face's node. */
void M3dColij_LineToThisItem(Item *pItem, SLineInfo *pInfo) {
	VECTOR PosnRelStart;
	SModel *pModel;
	SProjectedVertex *pVertices;

	geom_ld_mat(pc_geom.lcm, &pInfo->WorldCst);

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

	if (pModel->NumVertices > 128)
		pVertices = ProjectedVertices;
	else
		pVertices = (SProjectedVertex *)SCRATCHPAD;

	if (!*(int *)&pItem->Angles && !pItem->Angles.vz && !(pItem->Flags & 0x200)) {
		PosnRelStart.vx = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
		PosnRelStart.vy = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
		PosnRelStart.vz = (pItem->Position.vz - pInfo->StartCoords.vz) >> 12;
		geom_load_vector(pc_geom.tr, 0, 0, 0);
	} else {
		if (pInfo->DropDown) {
			Tfm.t[0] = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
			Tfm.t[1] = (pInfo->StartCoords.vz - pItem->Position.vz) >> 12;
			Tfm.t[2] = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
		} else {
			Tfm.t[0] = (pItem->Position.vx - pInfo->StartCoords.vx) >> 12;
			Tfm.t[1] = (pItem->Position.vy - pInfo->StartCoords.vy) >> 12;
			Tfm.t[2] = (pItem->Position.vz - pInfo->StartCoords.vz) >> 12;
			{
				/* through the colour matrix (16 bits per coordinate in) */
				int16_t v[3];
				GeomAcc r;

				geom_ld_lv(v, &Tfm.t[0]);
				r = geom_mvmva(pc_geom.lcm, 0, v[0], v[1], v[2], 12, 0, 0);
				geom_st_lv(&Tfm.t[0], r.mac);
			}
		}

		SetTransMatrix(&Tfm);

		PosnRelStart.vx = 0;
		PosnRelStart.vy = 0;
		PosnRelStart.vz = 0;

		if (*(int *)&pItem->Angles || pItem->Angles.vz) {
			M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &Tfm);

			if (pItem->Flags & 0x200)
				M3dMaths_ScaleMatrix(pItem, &Tfm);

			M3dMaths_MulLcmMatrix(&Tfm, &Tfm);
			geom_ld_mat(pc_geom.lcm, &Tfm);
		} else {
			M3dMaths_SetIdentityRotation(&Tfm);
			M3dMaths_ScaleMatrix(pItem, &Tfm);
			M3dMaths_MulLcmMatrix(&Tfm, &Tfm);
			geom_ld_mat(pc_geom.lcm, &Tfm);
		}
	}

	if (!(M3dAsm_LineColijProcessVertices(pModel, pVertices, pInfo->Length, &PosnRelStart) &
	      0x60f)) {
		CollidedTriggerModel = -1;
		M3dAsm_LineColijProcessFaces(pModel, pVertices, pInfo, pItem);

		if (CollidedTriggerModel != -1) {
			int i = Spool_GetEnvIndex(pItem->Region);

			FireNode(EnvModelChecksums[i][CollidedTriggerModel], i);
		}
	}
}

static inline void SetSVector(SVECTOR *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

/* M3dColij_LineToSphere__FRC7CVectorT0R7CVectorP5CBodyT3i (0x80080cf8) */
/* Fuzzy demo twin (0.72); locals from its MAIN.SYM. Finds the first body in
   the list whose bounding sphere the segment OldPos->NewPos passes through: an AABB reject, then
   the perpendicular distance (cross product with the unit direction) against the radius, then the
   distance along the line, with the end caps checked against the end points. The nearest hit's
   distance is kept in NearestHit's best field, and ColijPos is set to the hit point. */
CBody *M3dColij_LineToSphere(const CVector &OldPos, const CVector &NewPos, CVector &ColijPos,
                             CBody *pBody, CBody *pIgnore, int ScaleFactor) {
	int Length;
	int DotProd;
	CVector DirectionVec;
	CVector MinCoords;
	CVector MaxCoords;
	CVector SquareVec;
	VECTOR OldToCentre;
	VECTOR NewToCentre;
	SVECTOR DirVecShort;

	pBodyHit = 0;
	NearestHit.Dist = 0x7fffffff;
	DirectionVec = (NewPos - OldPos) >> 12;
	geom_square(&DirectionVec, &SquareVec);

	Length = SquareRoot0(SquareVec.vx + SquareVec.vy + SquareVec.vz);

	if (!Length)
		return 0;

	DirectionVec = (DirectionVec << 12) / Length;
	/* the direction stays loaded for the loop, as in retail: on the rotation's diagonal (for the
	 * cross products) and as the light matrix's first row (the dot products) */
	geom_load_op_diag(&pc_geom, DirectionVec.vx, DirectionVec.vy, DirectionVec.vz);
	SetSVector(&DirVecShort, DirectionVec.vx, DirectionVec.vy, DirectionVec.vz);
	pc_geom.llm[0][0] = DirVecShort.vx;
	pc_geom.llm[0][1] = DirVecShort.vy;
	pc_geom.llm[0][2] = DirVecShort.vz;
	pc_geom.llm[1][0] = DirVecShort.pad;

	if (OldPos.vx < NewPos.vx) {
		MinCoords.vx = OldPos.vx;
		MaxCoords.vx = NewPos.vx;
	} else {
		MinCoords.vx = NewPos.vx;
		MaxCoords.vx = OldPos.vx;
	}

	if (OldPos.vy < NewPos.vy) {
		MinCoords.vy = OldPos.vy;
		MaxCoords.vy = NewPos.vy;
	} else {
		MinCoords.vy = NewPos.vy;
		MaxCoords.vy = OldPos.vy;
	}

	if (OldPos.vz < NewPos.vz) {
		MinCoords.vz = OldPos.vz;
		MaxCoords.vz = NewPos.vz;
	} else {
		MinCoords.vz = NewPos.vz;
		MaxCoords.vz = OldPos.vz;
	}

	for (; pBody; pBody = (CBody *)pBody->mNextItem) {
		int Radius;
		int RadiusSquared;

		if (pBody->mCBodyFlags & CBODY_DEAD)
			continue;

		if (pBody == pIgnore)
			continue;

		Radius = pBody->mRMinor << 12;

		if (pBody->mPos.vx + Radius < MinCoords.vx || MaxCoords.vx < pBody->mPos.vx - Radius)
			continue;

		if (pBody->mPos.vy + Radius < MinCoords.vy || MaxCoords.vy < pBody->mPos.vy - Radius)
			continue;

		if (pBody->mPos.vz + Radius < MinCoords.vz || MaxCoords.vz < pBody->mPos.vz - Radius)
			continue;

		*(CVector *)&OldToCentre = (pBody->mPos - OldPos) >> 12;
		{
			/* the squares of direction x OldToCentre (16-bit components) */
			int16_t d[3], v[3];
			GeomAcc r;

			geom_op_diag(&pc_geom, d);
			geom_ld_lv(v, &OldToCentre);
			r = geom_op(d, v, 12, 0, 0);
			r = geom_sqr(r.ir, 0, 1, 0);
			geom_st_lv(&SquareVec, r.mac);
		}

		RadiusSquared = pBody->mRMinor * pBody->mRMinor;

		if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
			continue;

		{
			/* direction . OldToCentre */
			int16_t v[3];

			geom_ld_lv(v, &OldToCentre);
			DotProd = geom_mvmva(pc_geom.llm, 0, v[0], v[1], v[2], 12, 0, 0).mac[0];
		}

		if (DotProd >= NearestHit.Dist)
			continue;

		if (DotProd < 0) {
			geom_square(&OldToCentre, &SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		} else if (DotProd > Length) {
			*(CVector *)&NewToCentre = (pBody->mPos - NewPos) >> 12;
			geom_square(&NewToCentre, &SquareVec);

			if (SquareVec.vx + SquareVec.vy + SquareVec.vz >= RadiusSquared)
				continue;
		}

		pBodyHit = pBody;
		NearestHit.Dist = DotProd;
	}

	if (pBodyHit) {
		{
			/* direction * distance (16 bits each) */
			int16_t v[3];
			GeomAcc r;

			geom_ld_lv(v, &DirectionVec);
			r = geom_gpf((int16_t)NearestHit.Dist, v, 12, 0, 0, 0, 0);
			geom_st_lv(&ColijPos, r.mac);
		}
		ColijPos <<= 12;
		ColijPos += OldPos;
	}

	return pBodyHit;
}

/* FUN_80081234 (0x80081234) */
void FUN_80081234(void) {}
