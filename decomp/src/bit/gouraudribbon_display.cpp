/* Display__14CGouraudRibbon (0x8002dbf0) flags=-fstructure-noalias */
/* C++ (BIT.CPP). Fuzzy demo twin (0.97); locals from its MAIN.SYM. Draws a Gouraud-shaded ribbon:
   transforms each point into a scratchpad buffer (screen position, depth, width), computes
   averaged screen normals, then emits two semi-transparent POLY_G4 quads per segment (top and
   bottom half) plus a DR_TPAGE. After the first frames it adds a third quad between the last two
   frames' positions (the trail). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/bit.h"
#include "game/m3d.h"
#include "game/trig.h"

extern SViewport *pCurrentViewport;
extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

u_long StoredTPagePrim[2];

int Transform(const CVector &, int &);
void CalcScreenNormal(SCalcBuffer *, int &, int &, int);

void CGouraudRibbon::Display() {
	SRibbonPoint *pPoint;
	SCalcBuffer *pBuf;
	int LastNormalX;
	int LastNormalY;
	SCalcBuffer *pLastBuf;
	SRibbonPoint *pLastPoint;
	int x;
	int y;
	int dx;
	int dy;
	int LastTop;
	int LastBottom;

	pBuf = (SCalcBuffer *)0x1f800000;
	pPoint = mpPoints;

	{
		int i;

		for (i = 0; i < mNumPoints; i++) {
			pBuf->Zed = Transform(pPoint->Pos, pBuf->Pos);

			if (pBuf->Zed < pCurrentViewport->Hither || pCurrentViewport->Yon < pBuf->Zed) {
				pBuf->Bad = 1;
			} else {
				pBuf->Bad = 0;
				pBuf->ScrWidth = pPoint->Width * 400 / pBuf->Zed;
			}

			pBuf++;
			pPoint++;
		}
	}

	pBuf = (SCalcBuffer *)0x1f800000;
	CalcScreenNormal(pBuf, LastNormalX, LastNormalY, 2);

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf++;

	{
		int i;

		for (i = 1; i < mNumPoints - 1; i++) {
			int TempX;
			int TempY;

			CalcScreenNormal(pBuf, TempX, TempY, 2);

			pBuf->NormalX = (TempX + LastNormalX) / 2;
			pBuf->NormalY = (TempY + LastNormalY) / 2;
			pBuf++;
			LastNormalX = TempX;
			LastNormalY = TempY;
		}
	}

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf = (SCalcBuffer *)0x1f800000;
	dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
	dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
	x = (short)pBuf->Pos;
	y = pBuf->Pos >> 16;
	LastTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
	LastBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
	pLastBuf = (SCalcBuffer *)0x1f800000;
	pBuf = pLastBuf + 1;
	pLastPoint = mpPoints;
	pPoint = pLastPoint + 1;

	{
		int i;

		for (i = 1; i < mNumPoints; pLastBuf++, pBuf++, pLastPoint++, i++, pPoint++) {
			uint *pOTPos;

			if (!(pBuf->NormalX | pBuf->NormalY))
				continue;

			if (!(pLastBuf->NormalX | pLastBuf->NormalY))
				continue;

			{
				int NewTop;
				int NewBottom;
				POLY_G4 *pTopPoly;
				POLY_G4 *pBottomPoly;
				DR_TPAGE *ptpage;
				uchar *newppoly;
				uint a;
				uint b;

				pOTPos =
				    (uint *)((char *)pDoubleBuffer->pOT + ((pBuf->Zed - mFrigDeltaZ) & 0x3ffc));
				dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
				dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
				x = (short)pBuf->Pos;
				y = pBuf->Pos >> 16;
				NewTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
				NewBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
				pTopPoly = (POLY_G4 *)pPoly;
				pBottomPoly = pTopPoly + 1;
				ptpage = (DR_TPAGE *)(pBottomPoly + 1);
				newppoly = (uchar *)(ptpage + 1);

				if (newppoly > PolyBufferEnd)
					goto SEEYA;

				a = StoredTPagePrim[0];
				b = StoredTPagePrim[1];
				ptpage->tag = a;
				ptpage->code[0] = b;
				pPoly = newppoly;

				if (mTrail >= 3) {
					{
						uchar *newppoly = pPoly + sizeof(POLY_G4);

						if (newppoly > PolyBufferEnd)
							goto SEEYA;

						pPoly = newppoly;
					}

					{
						POLY_G4 *pTrail = (POLY_G4 *)newppoly;

						*(u_long *)&pTrail->tag = 0x08000000;
						*(u_long *)&pTrail->r0 = 0x3a000000;
						*(u_long *)&pTrail->r1 = 0;
						*(u_long *)&pTrail->r3 = *(uint *)&pLastPoint->r;
						*(u_long *)&pTrail->r2 = *(uint *)&pPoint->r;
						*(u_long *)&pTrail->x1 = pLastPoint->Last2Scr;
						*(u_long *)&pTrail->x0 = pPoint->Last2Scr;
						*(u_long *)&pTrail->x3 = pLastBuf->Pos;
						*(u_long *)&pTrail->x2 = pBuf->Pos;
						addPrim(pOTPos, pTrail);
					}
				}

				*(u_long *)&pTopPoly->tag = *(u_long *)&pBottomPoly->tag = 0x08000000;
				*(u_long *)&pTopPoly->r0 = *(u_long *)&pBottomPoly->r0 = 0x3a000000;
				*(u_long *)&pTopPoly->r1 = 0;
				*(u_long *)&pTopPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pTopPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pTopPoly->x0 = NewTop;
				*(u_long *)&pTopPoly->x1 = LastTop;
				*(u_long *)&pTopPoly->x2 = pBuf->Pos;
				*(u_long *)&pTopPoly->x3 = pLastBuf->Pos;
				addPrim(pOTPos, pTopPoly);

				*(u_long *)&pBottomPoly->r1 = 0;
				*(u_long *)&pBottomPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pBottomPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pBottomPoly->x0 = NewBottom;
				*(u_long *)&pBottomPoly->x1 = LastBottom;
				*(u_long *)&pBottomPoly->x2 = pBuf->Pos;
				LastBottom = NewBottom;
				*(u_long *)&pBottomPoly->x3 = pLastBuf->Pos;
				addPrim(pOTPos, pBottomPoly);

				LastTop = NewTop;
				addPrim(pOTPos, ptpage);
			}
		}
	}
SEEYA:
	if (mTrail) {
		int i;

		pBuf = (SCalcBuffer *)0x1f800000;
		pPoint = mpPoints;

		for (i = 0; i < mNumPoints; i++) {
			if (mTrail == 1) {
				pPoint->Last2Scr = pPoint->Last1Scr = pBuf->Pos;
			} else {
				pPoint->Last2Scr = pPoint->Last1Scr;
				pPoint->Last1Scr = pBuf->Pos;
			}

			pPoint++;
			pBuf++;
		}

		mTrail++;
	}
}
