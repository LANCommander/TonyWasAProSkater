/* Display__15CTexturedRibbon (0x8002e31c) flags=-fstructure-noalias */
/* C++ (BIT.CPP). Fuzzy demo twin (0.99); locals and blocks from its MAIN.SYM. The textured sibling
   of CGouraudRibbon::Display: transforms the points into the scratchpad (screen position, depth,
   inner and outer widths), smooths the screen normals, then per segment emits four POLY_GT4s
   (inner and outer strips, top and bottom), bracketed by texture-window on/off prims and a
   DR_TPAGE. The upper half of a wibbling ribbon gets sine offsets on its vertices.
   Near-miss (685/695): in the wibble block the sine-table address is built through v1
   (`lui v1; addiu t0,v1`), and gcc's delay-slot filler hoists that lui; retail builds it in t0. */
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
extern int TTime;
extern SSinCos
    rcossin_tbl[1]; /* declared <= 8 bytes: cc1 then emits one la (as retail), not a hi/lo split */

u_long StoredTPagePrim[2];

int Transform(const CVector &, int &);
void CalcScreenNormal(SCalcBuffer *, int &, int &, int);

void CTexturedRibbon::Display() {
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
	int dxB;
	int dyB;
	int LastTop;
	int LastBottom;
	int LastTopB;
	int LastBottomB;
	int voff;

	if (mDontDisplay)
		return;

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
				pBuf->ScrWidthB = pPoint->WidthB * 400 / pBuf->Zed;
			}

			pBuf++;
			pPoint++;
		}
	}

	LastNormalX = 0;
	pBuf = (SCalcBuffer *)0x1f800000;
	LastNormalY = 0;

	{
		int i;

		for (i = 0; i < mNumPoints - 1; i++) {
			CalcScreenNormal(pBuf, pBuf->NormalX, pBuf->NormalY, 4);

			if (!pBuf->NormalX && !pBuf->NormalY) {
				pBuf->NormalX = LastNormalX;
				pBuf->NormalY = LastNormalY;
			}

			LastNormalX = pBuf->NormalX;
			LastNormalY = pBuf->NormalY;
			pBuf++;
		}
	}

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf = (SCalcBuffer *)0x1f800000;
	dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
	dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
	dxB = pBuf->NormalX * pBuf->ScrWidthB >> 6;
	dyB = pBuf->NormalY * pBuf->ScrWidthB >> 6;
	x = (short)pBuf->Pos;
	y = pBuf->Pos >> 16;
	pLastBuf = (SCalcBuffer *)0x1f800000;
	voff = mV;
	LastTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
	LastBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
	LastTopB = ((y + dyB) << 16) | ((x + dxB) & 0xffff);
	LastBottomB = ((y - dyB) << 16) | ((x - dxB) & 0xffff);
	pBuf = pLastBuf + 1;
	pLastPoint = mpPoints;
	pPoint = pLastPoint + 1;

	{
		int i;

		for (i = 1; i < mNumPoints; pLastBuf++, pBuf++, pLastPoint++, pPoint++, i++) {
			uint *pOTPos;

			if (!(pBuf->NormalX | pBuf->NormalY))
				continue;

			if (!(pLastBuf->NormalX | pLastBuf->NormalY))
				continue;

			{
				int MinZ;
				int NewTop;
				int NewBottom;
				int NewTopB;
				int NewBottomB;
				POLY_GT4 *pTopPoly;
				POLY_GT4 *pBottomPoly;
				POLY_GT4 *pTopPolyB;
				POLY_GT4 *pBottomPolyB;
				DR_TWIN *pWinOn;
				DR_TWIN *pWinOff;
				DR_TPAGE *ptpage;
				uchar *newppoly;
				RECT win;
				int u;
				int v;
				uint a;
				uint b;

				MinZ = pLastBuf->Zed;

				if (pBuf->Zed < MinZ)
					MinZ = pBuf->Zed;

				dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
				dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
				dxB = pBuf->NormalX * pBuf->ScrWidthB >> 6;
				dyB = pBuf->NormalY * pBuf->ScrWidthB >> 6;
				x = (short)pBuf->Pos;
				y = pBuf->Pos >> 16;
				NewTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
				NewBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
				pOTPos = (uint *)((char *)pDoubleBuffer->pOT + (MinZ & 0x3ffc));
				NewTopB = ((y + dyB) << 16) | ((x + dxB) & 0xffff);
				NewBottomB = ((y - dyB) << 16) | ((x - dxB) & 0xffff);

				if (mWibble && i >= mNumPoints / 2) {
					NewTop = ((y + dy + rcossin_tbl[(TTime * 200 + 130) & 4095].s / 2048) << 16) |
					         ((x + dx) & 0xffff);
					y += rcossin_tbl[(TTime * 250) & 4094].s * 3 / 4096;
					pBuf->Pos = (pBuf->Pos & 0xffff) | (y << 16);
					NewBottom =
					    ((y - dy + rcossin_tbl[(TTime * 150 + 100) & 4095].s / 2048) << 16) |
					    ((x - dx) & 0xffff);
				}

				pTopPoly = (POLY_GT4 *)pPoly;
				pBottomPoly = pTopPoly + 1;
				pTopPolyB = pBottomPoly + 1;
				pBottomPolyB = pTopPolyB + 1;
				pWinOn = (DR_TWIN *)(pBottomPolyB + 1);
				pWinOff = pWinOn + 1;
				ptpage = (DR_TPAGE *)(pWinOff + 1);
				newppoly = (uchar *)(ptpage + 1);

				if (newppoly > PolyBufferEnd)
					return;

				pPoly = newppoly;
				win.x = win.y = win.w = win.h = 0;
				setTexWindow(pWinOff, &win);

				win.x = mpTex->u0;
				win.y = mpTex->v0;
				win.w = 64;
				win.h = 64;
				setTexWindow(pWinOn, &win);
				addPrim(pOTPos, pWinOff);

				*(u_long *)&pTopPoly->tag = *(u_long *)&pBottomPoly->tag =
				    *(u_long *)&pTopPolyB->tag = *(u_long *)&pBottomPolyB->tag = 0x0c000000;
				pTopPoly->clut = pBottomPoly->clut = pTopPolyB->clut = pBottomPolyB->clut =
				    mpTex->clut;
				pTopPoly->tpage = pBottomPoly->tpage = pTopPolyB->tpage = pBottomPolyB->tpage =
				    mpTex->tpage;
				u = mpTex->u0 + (mU & 63);
				v = mpTex->v0 + (voff & 63);
				pTopPolyB->u0 = u;
				pTopPolyB->v0 = v;
				pTopPolyB->u2 = u + 16;
				pTopPolyB->v2 = v;
				pTopPolyB->u1 = u;
				pTopPolyB->v1 = v + mDv;
				pTopPolyB->u3 = u + 16;
				pTopPolyB->v3 = v + mDv;
				pTopPoly->u0 = u + 16;
				pTopPoly->v0 = v;
				pTopPoly->u2 = u + 32;
				pTopPoly->v2 = v;
				pTopPoly->u1 = u + 16;
				pTopPoly->v1 = v + mDv;
				pTopPoly->u3 = u + 32;
				pTopPoly->v3 = v + mDv;
				pBottomPoly->u0 = u + 48;
				pBottomPoly->v0 = v;
				pBottomPoly->u2 = u + 32;
				pBottomPoly->v2 = v;
				pBottomPoly->u1 = u + 48;
				pBottomPoly->v1 = v + mDv;
				pBottomPoly->u3 = u + 32;
				pBottomPoly->v3 = v + mDv;
				pBottomPolyB->u0 = u + 64;
				pBottomPolyB->v0 = v;
				pBottomPolyB->u2 = u + 48;
				pBottomPolyB->v2 = v;
				pBottomPolyB->u1 = u + 64;
				pBottomPolyB->v1 = v + mDv;
				pBottomPolyB->u3 = u + 48;
				pBottomPolyB->v3 = v + mDv;
				voff -= mDv;
				*(u_long *)&pTopPoly->r0 = *(uint *)&pPoint->rB;
				*(u_long *)&pTopPoly->r1 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pTopPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pTopPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pTopPoly->x0 = NewTop;
				*(u_long *)&pTopPoly->x1 = LastTop;
				*(u_long *)&pTopPoly->x2 = pBuf->Pos;
				*(u_long *)&pTopPoly->x3 = pLastBuf->Pos;
				*(u_long *)&pTopPolyB->r0 = 0;
				*(u_long *)&pTopPolyB->r1 = 0;
				*(u_long *)&pTopPolyB->r2 = *(uint *)&pPoint->rB;
				*(u_long *)&pTopPolyB->r3 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pTopPolyB->x0 = NewTopB;
				*(u_long *)&pTopPolyB->x1 = LastTopB;
				*(u_long *)&pTopPolyB->x2 = NewTop;
				*(u_long *)&pTopPolyB->x3 = LastTop;
				*(u_long *)&pBottomPoly->r0 = *(uint *)&pPoint->rB;
				*(u_long *)&pBottomPoly->r1 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pBottomPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pBottomPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pBottomPoly->x0 = NewBottom;
				*(u_long *)&pBottomPoly->x1 = LastBottom;
				*(u_long *)&pBottomPoly->x2 = pBuf->Pos;
				*(u_long *)&pBottomPoly->x3 = pLastBuf->Pos;
				*(u_long *)&pBottomPolyB->r0 = 0;
				*(u_long *)&pBottomPolyB->r1 = 0;
				*(u_long *)&pBottomPolyB->r2 = *(uint *)&pPoint->rB;
				*(u_long *)&pBottomPolyB->r3 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pBottomPolyB->x0 = NewBottomB;
				*(u_long *)&pBottomPolyB->x1 = LastBottomB;
				*(u_long *)&pBottomPolyB->x2 = NewBottom;
				*(u_long *)&pBottomPolyB->x3 = LastBottom;
				pTopPoly->code = pBottomPoly->code = pTopPolyB->code = pBottomPolyB->code = 0x3e;
				addPrim(pOTPos, pTopPoly);
				addPrim(pOTPos, pTopPolyB);
				addPrim(pOTPos, pBottomPoly);
				addPrim(pOTPos, pBottomPolyB);

				LastTop = NewTop;
				addPrim(pOTPos, pWinOn);

				LastTopB = NewTopB;
				a = StoredTPagePrim[0];
				b = StoredTPagePrim[1];
				ptpage->tag = a;
				ptpage->code[0] = b;
				addPrim(pOTPos, ptpage);

				LastBottom = NewBottom;
				LastBottomB = NewBottomB;
			}
		}
	}
}
