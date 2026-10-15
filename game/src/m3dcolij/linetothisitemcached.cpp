/* M3dColij_LineToThisItemCached__FP4ItemP9SLineInfoP9SColCache (0x8007ef68) */
/* C++ (M3DCOLIJ.CPP). Fuzzy demo twin (0.52); locals from its MAIN.SYM. Tests the line against
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
#include "game/colij.h"
#include "game/m3dasm.h"
#include "game/trig.h"
#include "game/vector.h"

extern SPSXRegion PSXRegion[];
extern SFaceCache FaceCache[];
extern SHit NearestHit;
extern int OtherEnvIndex;
extern int *EnvModelChecksums[2];

extern int TriggerCollisionCheck;
extern int M3dColij_ZeroMask;
extern int M3dColij_OneMask;

int Spool_GetEnvIndex(int);

extern "C" {
	int Mips_DotProd2(int, int, int, int);
}

static inline void FireNode(int node, int env) {
	OtherEnvIndex = env ^ 1;
	TriggerCollisionCheck = 0;
	Trig_TriggerCommandPoint(node, 1);
}

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
