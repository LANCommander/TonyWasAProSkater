/* Bit_Display__Fv (0x8002ab20) flags=-fstructure-noalias */
/* C++ (BIT.CPP). Demo twin (0.991); locals and blocks from its MAIN.SYM. Draws every effect list
   (special, glow, gouraud/flat polylines, flat bits, glass, gouraud lines, quads, chunks, pixels,
   linked two-ended bits) into the primitive buffer.
   Near-miss (2979/3030 aligned): the glow's first OT/RadA block schedules the OT chain later than
   retail (insns 207-235), and in the flat-bit block Angle/Width/OffX get different registers.
   That one is local-alloc order in the !Angle block: retail gives the Height/OffY/Width product
   temps v0/v1/a0 and the OffX (short) shift temp a1 (so global puts OffX in a1, Width in t5,
   Angle in s3); ours allocates the OffX chain first (v0). Statement order follows the demo lines. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
	#include <inline_c.h>
}

#include "game/bit.h"
#include "game/m3d.h"
#include "game/trig.h"
#include "game/camera.h"

extern SCamera MikeCamera[];
extern int Camera_CurrentMikeCamera;
extern SViewport *pCurrentViewport;
extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;
extern SSinCos rcossin_tbl[];

CSpecialDisplay *SpecialDisplayList;
CGlow *GlowList;
CGPolyLine *GPolyLineList;
CPolyLine *PolyLineList;
CFlatBit *FlatBitList;
CGlassBit *GlassList;
CGLine *GLineList;
CQuadBit *QuadBitList;
CChunkBit *ChunkBitList;
CPixel *PixelList;
CLinked2EndedBit *Linked2EndedBitList;
u_long StoredTPagePrim[2];

int M3dAsm_ClipLine(SClipLineBuf *);
int M3dMaths_SquareRoot0(int);

void Bit_Display() {
	CVector Temp;
	CVector camera_pos;
	int GTEZedLimit;
	CSpecialDisplay *pSpecial;
	CGlow *pGlow;

	gte_SetRotMatrix(&MikeCamera[Camera_CurrentMikeCamera].View);
	__asm__ volatile("ctc2 $0, $5; ctc2 $0, $6; ctc2 $0, $7");

	camera_pos.vx = MikeCamera[Camera_CurrentMikeCamera].Position.vx;
	camera_pos.vy = MikeCamera[Camera_CurrentMikeCamera].Position.vy;
	camera_pos.vz = MikeCamera[Camera_CurrentMikeCamera].Position.vz;
	GTEZedLimit = pCurrentViewport->Zoom >> 1;
	pSpecial = SpecialDisplayList;

	while (pSpecial) {
		pSpecial->Display();

		pSpecial = (CSpecialDisplay *)pSpecial->mNext;
	}

	{
		for (pGlow = GlowList; pGlow; pGlow = (CGlow *)pGlow->mNext) {
			uint y0x0;
			VECTOR RotCoords;
			CVector Centre = (pGlow->mPos >> 12) - camera_pos;

			gte_ldlv0(&Centre);
			gte_rtps();
			DR_TPAGE *ptpage = (DR_TPAGE *)pPoly;
			uchar *newppoly = pPoly + sizeof(DR_TPAGE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			uint a = StoredTPagePrim[0];
			uint b = StoredTPagePrim[1];

			ptpage->tag = a;
			ptpage->code[0] = b;
			int n = pGlow->mNumSections;
			int Step = pGlow->mStepAngle;
			int Ang = pGlow->mAngle;
			SSection *pSection = pGlow->mpSections;

			gte_stlvnl(&RotCoords);

			if ((uint)RotCoords.vz > 20000)
				continue;

			short CentreX;
			short CentreY;
			int RGBFade = 0;

			if (RotCoords.vz < 700)
				RGBFade = RotCoords.vz * 255 / 700 + 1;

			if (RotCoords.vz < GTEZedLimit) {
				CentreX =
				    pCurrentViewport->Zoom * RotCoords.vx / RotCoords.vz + pCurrentViewport->xC;

				if (CentreX > 1023)
					CentreX = 1023;
				else if (CentreX < -1024)
					CentreX = -1024;

				CentreY =
				    pCurrentViewport->Zoom * RotCoords.vy / RotCoords.vz + pCurrentViewport->yC;

				if (CentreY > 1023)
					CentreY = 1023;
				else if (CentreY < -1024)
					CentreY = -1024;
			} else {
				gte_stsxy(&y0x0);

				CentreX = (short)y0x0;
				CentreY = (short)(y0x0 >> 16);
			}

			if (CentreX == -1024 || CentreX == 1023 || CentreY == -1024 || CentreY == 1023)
				continue;

			uint *OTPos = (uint *)((char *)pDoubleBuffer->pOT + (RotCoords.vz & 0x3ffc));
			DVECTOR *pBufA = (DVECTOR *)0x1f800000;
			DVECTOR *pBufB = (DVECTOR *)0x1f800004 + pGlow->mNumFringes;
			SSinCos *pTab = rcossin_tbl;
			SSinCos *p = &pTab[Ang & 4095];
			int RadA = pSection[n - 1].Radius * 400 / RotCoords.vz;
			int MSin = p->s * 400 / 256;
			int MCos = p->c;
			int x = CentreX + RadA * MSin / 4096;
			int y = CentreY - RadA * MCos / 4096;

			if (x > 612)
				x = 612;

			if (x < -100)
				x = -100;

			if (y > 340)
				y = 340;

			if (y < -100)
				y = -100;

			int Rad;
			SFringeQuad *pFringe;

			{
				int f;

				pBufA->vx = x;
				pBufA->vy = y;
				Rad = pSection[n - 1].Radius;
				pFringe = pGlow->mpFringes + n - 1;

				for (f = 0; f < pGlow->mNumFringes; f++) {
					Rad += pFringe->Width;
					x = CentreX + (Rad * 400 / RotCoords.vz) * MSin / 4096;
					y = CentreY - (Rad * 400 / RotCoords.vz) * MCos / 4096;

					if (x > 612)
						x = 612;

					if (x < -100)
						x = -100;

					if (y > 340)
						y = 340;

					if (y < -100)
						y = -100;

					pBufA[f + 1].vx = x;
					pBufA[f + 1].vy = y;
					pFringe += n;
				}
			}

			{
				int i;
				int f;

				Ang += Step;

				for (i = 0; i < n; i++) {
					int DoDisplay;

					DoDisplay = (pGlow->mMask >> i) & 1;

					if (i == n - 1)
						Ang = pGlow->mAngle;

					int RadB = pSection->Radius * 400 / RotCoords.vz;
					SSinCos *q = (SSinCos *)((Ang & 4095) * 4 + (uint)pTab);

					MSin = q->s * 400 / 256;
					MCos = q->c;
					x = CentreX + RadB * MSin / 4096;
					y = CentreY - RadB * MCos / 4096;

					if (x > 612)
						x = 612;

					if (x < -100)
						x = -100;

					if (y > 340)
						y = 340;

					if (y < -100)
						y = -100;

					pBufB->vx = x;
					pBufB->vy = y;

					if (!pGlow->mSkipTriangles) {
						if (DoDisplay) {
							POLY_G3 *aGou;
							uchar *newppoly;

							aGou = (POLY_G3 *)pPoly;
							newppoly = pPoly + sizeof(POLY_G3);

							if (newppoly > PolyBufferEnd)
								return;

							pPoly = newppoly;
							aGou->tag = 0x06000000;
							*(u_long *)&aGou->r0 = pGlow->mCentreCodeBGR;
							*(u_long *)&aGou->r1 = pSection->CodeBGR;
							*(u_long *)&aGou->r2 = pSection->CodeBGR;

							if (RGBFade) {
								aGou->r0 = aGou->r0 * RGBFade >> 8;
								aGou->g0 = aGou->g0 * RGBFade >> 8;
								aGou->b0 = aGou->b0 * RGBFade >> 8;
								aGou->r1 = aGou->r1 * RGBFade >> 8;
								aGou->g1 = aGou->g1 * RGBFade >> 8;
								aGou->b1 = aGou->b1 * RGBFade >> 8;
								aGou->r2 = aGou->r2 * RGBFade >> 8;
								aGou->g2 = aGou->g2 * RGBFade >> 8;
								aGou->b2 = aGou->b2 * RGBFade >> 8;
							}

							aGou->x0 = CentreX;
							aGou->y0 = CentreY;
							aGou->x1 = pBufA->vx;
							aGou->y1 = pBufA->vy;
							aGou->x2 = pBufB->vx;
							aGou->y2 = pBufB->vy;
							addPrim(OTPos, aGou);
						}
					}

					{
						int f;

						Rad = pSection->Radius;
						pFringe = pGlow->mpFringes + i;

						for (f = 0; f < pGlow->mNumFringes; f++) {
							Rad += pFringe->Width;
							x = CentreX + (Rad * 400 / RotCoords.vz) * MSin / 4096;
							y = CentreY - (Rad * 400 / RotCoords.vz) * MCos / 4096;

							if (x > 612)
								x = 612;

							if (x < -100)
								x = -100;

							if (y > 340)
								y = 340;

							if (y < -100)
								y = -100;

							pBufB[f + 1].vx = x;
							pBufB[f + 1].vy = y;

							if (DoDisplay) {
								POLY_G4 *aGou;
								uchar *newppoly;
								uint CodeBGR;

								aGou = (POLY_G4 *)pPoly;
								newppoly = pPoly + sizeof(POLY_G4);

								if (newppoly > PolyBufferEnd)
									return;

								pPoly = newppoly;
								aGou->tag = 0x08000000;
								CodeBGR = pFringe->CodeBGR;
								*(u_long *)&aGou->r0 = CodeBGR;
								*(u_long *)&aGou->r1 = CodeBGR;

								if (f) {
									uint PadBGR;

									PadBGR = (pFringe - n)->CodeBGR;
									*(u_long *)&aGou->r2 = PadBGR;
									*(u_long *)&aGou->r3 = PadBGR;
								} else {
									*(u_long *)&aGou->r2 = pSection->CodeBGR;
									*(u_long *)&aGou->r3 = pSection->CodeBGR;
								}

								if (RGBFade) {
									aGou->r0 = aGou->r0 * RGBFade >> 8;
									aGou->g0 = aGou->g0 * RGBFade >> 8;
									aGou->b0 = aGou->b0 * RGBFade >> 8;
									aGou->r1 = aGou->r1 * RGBFade >> 8;
									aGou->g1 = aGou->g1 * RGBFade >> 8;
									aGou->b1 = aGou->b1 * RGBFade >> 8;
									aGou->r2 = aGou->r2 * RGBFade >> 8;
									aGou->g2 = aGou->g2 * RGBFade >> 8;
									aGou->b2 = aGou->b2 * RGBFade >> 8;
									aGou->r3 = aGou->r3 * RGBFade >> 8;
									aGou->g3 = aGou->g3 * RGBFade >> 8;
									aGou->b3 = aGou->b3 * RGBFade >> 8;
								}

								aGou->x2 = pBufA[f].vx;
								aGou->y2 = pBufA[f].vy;
								aGou->x3 = pBufB[f].vx;
								aGou->y3 = pBufB[f].vy;
								aGou->x0 = pBufA[f + 1].vx;
								aGou->y0 = pBufA[f + 1].vy;
								aGou->x1 = pBufB[f + 1].vx;
								aGou->y1 = pBufB[f + 1].vy;
								addPrim(OTPos, aGou);
							}

							pFringe += n;
						}
					}

					{
						DVECTOR *pTemp;

						pTemp = pBufA;
						pBufA = pBufB;
						pBufB = pTemp;
					}

					Ang += Step;
					pSection++;
				}
			}

			addPrim(OTPos, ptpage);
		}
	}

	CGPolyLine *pGPolyLine;

	{
		uint LastBGR;
		SLineSeg *pSeg;
		SClipLineBuf *pClip = (SClipLineBuf *)0x1f800000;

		pGPolyLine = GPolyLineList;

		while (pGPolyLine) {
			int i;

			LastBGR = *(uint *)&pGPolyLine->mStartR;
			pClip->Start.vx = pGPolyLine->mStart.vx;
			pClip->Start.vy = pGPolyLine->mStart.vy;
			pClip->Start.vz = pGPolyLine->mStart.vz;
			pSeg = pGPolyLine->mSegs;

			for (i = 0; i < pGPolyLine->mNumSegs; i++) {
				uint NewBGR;
				int sort;

				NewBGR = *(uint *)&pSeg->r;
				pClip->End.vx = pSeg->End.vx;
				pClip->End.vy = pSeg->End.vy;
				pClip->End.vz = pSeg->End.vz;
				sort = M3dAsm_ClipLine(pClip);

				if (sort >= 0) {
					DR_TPAGE *ptpage;
					LINE_G2 *aline;
					uchar *newppoly;
					uint *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_G2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_G2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					aline->tag = 0x04000000;
					*(u_long *)&aline->r0 = LastBGR;
					*(u_long *)&aline->r1 = NewBGR;

					if (pGPolyLine->mForcedPriority)
						otpos = (uint *)pDoubleBuffer->pOT + pGPolyLine->mForcedPriority;
					else
						otpos = (uint *)pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (LastBGR & 0x02000000) {
						uint a;
						uint b;

						a = StoredTPagePrim[0];
						b = StoredTPagePrim[1];
						ptpage->tag = a;
						ptpage->code[0] = b;
						addPrim(otpos, ptpage);
					}
				}

				LastBGR = NewBGR;
				pClip->Start.vx = pClip->End.vx;
				pClip->Start.vy = pClip->End.vy;
				pClip->Start.vz = pClip->End.vz;
				pSeg++;
			}

			pGPolyLine = (CGPolyLine *)pGPolyLine->mNext;
		}
	}

	CPolyLine *pPolyLine;

	{
		SLineSeg *pSeg;
		SClipLineBuf *pClip = (SClipLineBuf *)0x1f800000;

		pPolyLine = PolyLineList;

		while (pPolyLine) {
			int i;

			pClip->Start.vx = pPolyLine->mStart.vx;
			pClip->Start.vy = pPolyLine->mStart.vy;
			pClip->Start.vz = pPolyLine->mStart.vz;
			pSeg = pPolyLine->mSegs;

			for (i = 0; i < pPolyLine->mNumSegs; i++) {
				uint CodeBGR;
				int sort;

				CodeBGR = *(uint *)&pSeg->r;
				pClip->End.vx = pSeg->End.vx;
				pClip->End.vy = pSeg->End.vy;
				pClip->End.vz = pSeg->End.vz;
				sort = M3dAsm_ClipLine(pClip);

				if (sort >= 0) {
					DR_TPAGE *ptpage;
					LINE_F2 *aline;
					uchar *newppoly;
					uint *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_F2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_F2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					aline->tag = 0x03000000;
					*(u_long *)&aline->r0 = CodeBGR;

					if (pPolyLine->mForcedPriority)
						otpos = (uint *)pDoubleBuffer->pOT + pPolyLine->mForcedPriority;
					else
						otpos = (uint *)pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (CodeBGR & 0x02000000) {
						uint a;
						uint b;

						a = StoredTPagePrim[0];
						b = StoredTPagePrim[1];
						ptpage->tag = a;
						ptpage->code[0] = b;
						addPrim(otpos, ptpage);
					}
				}

				pClip->Start.vx = pClip->End.vx;
				pClip->Start.vy = pClip->End.vy;
				pClip->Start.vz = pClip->End.vz;
				pSeg++;
			}

			pPolyLine = (CPolyLine *)pPolyLine->mNext;
		}
	}

	CFlatBit *pFlatBit;

	for (pFlatBit = FlatBitList; pFlatBit; pFlatBit = (CFlatBit *)pFlatBit->mNext) {
		long zed;
		uint y0x0;

		Temp.vx = (pFlatBit->mPos.vx >> 12) - camera_pos.vx;
		Temp.vy = (pFlatBit->mPos.vy >> 12) - camera_pos.vy;
		Temp.vz = (pFlatBit->mPos.vz >> 12) - camera_pos.vz;
		gte_ldlv0(&Temp);
		gte_rtps();
		POLY_FT4 *apoly = (POLY_FT4 *)pPoly;
		uchar *newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		apoly->tag = 0x09000000;
		*(u_long *)&apoly->r0 = pFlatBit->mCodeBGR;
		gte_stlvnl2(&zed);

		if (zed < GTEZedLimit || zed > pCurrentViewport->Yon) {
			pPoly = (uchar *)apoly;
			continue;
		}

		uint Scale = pFlatBit->mScale;

		Scale = (Scale << 7) / zed;

		if (Scale > 512 && !(pFlatBit->mBitFlags & 8)) {
			pPoly = (uchar *)apoly;
			continue;
		}

		gte_stsxy(&y0x0);
		Texture *stored = pFlatBit->mpPSXFrame->pTexture;
		uint foo1 = *(uint *)&stored->u0;
		uint foo2 = *(uint *)&stored->u1;
		uint foo3 = *(uint *)&stored->u2;
		ushort foo4 = *(ushort *)&stored->u3;

		if (pFlatBit->mCLUT)
			*(u_long *)&apoly->u0 = (foo1 & 0xffff) | (pFlatBit->mCLUT << 16);
		else
			*(u_long *)&apoly->u0 = foo1;

		*(u_long *)&apoly->u1 = (foo2 & 0xff9fffff) | (pFlatBit->mSemiTransparencyRate << 16);
		*(u_long *)&apoly->u2 = foo3;
		*(ushort *)&apoly->u3 = foo4;
		short Angle = pFlatBit->mAngle;

		if (Scale >= 256 || Angle) {
			apoly->u1--;
			apoly->v2--;
			apoly->u3--;
			apoly->v3--;
		}

		uint *ptr;
		uchar Flipped;
		int OffX;
		int OffY;
		uint Width;
		uint Height;
		uint x_post_scale;
		uint y_post_scale;

		{
			ptr = (uint *)&apoly->x0;
			Flipped = pFlatBit->mBitFlags & 1;
			OffX = pFlatBit->mpPSXFrame->OffX;
			OffY = pFlatBit->mpPSXFrame->OffY;
			Width = pFlatBit->mpPSXFrame->Width;
			Height = pFlatBit->mpPSXFrame->Height;
			x_post_scale = pFlatBit->mPostScale & 0xffff;
			y_post_scale = pFlatBit->mPostScale >> 16;
			uint xs = Scale * x_post_scale;
			uint ys = Scale * y_post_scale;

			if (!Angle) {
				int Y1;

				OffX = (short)(Scale * OffX >> 7);
				Y1 = (Scale * OffY >> 7) << 16;
				y0x0 += Y1 + OffX;
				Width *= Scale;
				Width >>= 7;
				Height = Scale * Height >> 7;
				Height <<= 16;

				if (Width < 5)
					Width = 2;

				if (Height < 0x50000)
					Height = 0x10000;

				if (!Flipped) {
					*(uint *)&apoly->x0 = y0x0;
					ptr[2] = y0x0 + Width;
					ptr[4] = y0x0 + Height;
					ptr[6] = y0x0 + Width + Height;
				} else {
					ptr[2] = y0x0;
					*(uint *)&apoly->x0 = y0x0 + Width;
					ptr[6] = y0x0 + Height;
					ptr[4] = y0x0 + Width + Height;
				}
			} else {
				int sin;
				int cos;
				int xsin;
				int ysin;
				int xcos;
				int ycos;
				int xwsin;
				int yhsin;
				int xwcos;
				int yhcos;
				uint x_scale;
				uint y_scale;

				{
					SSinCos *p = rcossin_tbl;

					p += Angle & 4095;
					sin = p->s;
					cos = p->c;
				}

				xsin = OffX * sin;
				ysin = OffY * sin;
				xcos = OffX * cos;
				ycos = OffY * cos;
				xwsin = xsin + Width * sin;
				yhsin = ysin + Height * sin;
				xwcos = xcos + Width * cos;
				yhcos = ycos + Height * cos;
				x_scale = xs >> 12;
				y_scale = ys >> 12;

				if (!Flipped) {
					*(uint *)&apoly->x0 = y0x0 + (x_scale * (xcos - ysin) >> 19) +
					                      ((y_scale * (xsin + ycos) >> 19) << 16);
					ptr[2] = y0x0 + (x_scale * (xwcos - ysin) >> 19) +
					         ((y_scale * (xwsin + ycos) >> 19) << 16);
					ptr[4] = y0x0 + (x_scale * (xcos - yhsin) >> 19) +
					         ((y_scale * (xsin + yhcos) >> 19) << 16);
					ptr[6] = y0x0 + (x_scale * (xwcos - yhsin) >> 19) +
					         ((y_scale * (xwsin + yhcos) >> 19) << 16);
				} else {
					ptr[2] = y0x0 + (x_scale * (xcos - ysin) >> 19) +
					         ((y_scale * (xsin + ycos) >> 19) << 16);
					*(uint *)&apoly->x0 = y0x0 + (x_scale * (xwcos - ysin) >> 19) +
					                      ((y_scale * (xwsin + ycos) >> 19) << 16);
					ptr[6] = y0x0 + (x_scale * (xcos - yhsin) >> 19) +
					         ((y_scale * (xsin + yhcos) >> 19) << 16);
					ptr[4] = y0x0 + (x_scale * (xwcos - yhsin) >> 19) +
					         ((y_scale * (xwsin + yhcos) >> 19) << 16);
				}
			}
		}

		if (pFlatBit->mForcedPriority)
			addPrim((uint *)pDoubleBuffer->pOT + pFlatBit->mForcedPriority, apoly);
		else
			addPrim((uint *)(((zed - pFlatBit->mFrigDeltaZ) & 0x3ffc) + (uint)pDoubleBuffer->pOT),
			        apoly);
	}

	CGlassBit *pGlass;

	pGlass = GlassList;

	while (pGlass) {
		SVECTOR STemp;
		long zed;
		POLY_F3 *aGou;
		DR_TPAGE *ptpage;
		uchar *newppoly;
		uint a;
		uint b;
		uint *OTPos;

		STemp.vx = (pGlass->mPosA.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosA.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosA.vz >> 12) - camera_pos.vz;
		gte_ldv0(&STemp);

		STemp.vx = (pGlass->mPosB.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosB.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosB.vz >> 12) - camera_pos.vz;
		gte_ldv1(&STemp);

		STemp.vx = (pGlass->mPosC.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosC.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosC.vz >> 12) - camera_pos.vz;
		gte_ldv2(&STemp);
		gte_rtpt();

		aGou = (POLY_F3 *)pPoly;
		ptpage = (DR_TPAGE *)(aGou + 1);
		newppoly = pPoly + sizeof(POLY_F3) + sizeof(DR_TPAGE);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		aGou->tag = 0x04000000;
		aGou->code = 0x22;
		aGou->r0 = pGlass->mR;
		aGou->g0 = pGlass->mG;
		aGou->b0 = pGlass->mB;
		a = StoredTPagePrim[0];
		b = StoredTPagePrim[1];
		ptpage->tag = a;
		ptpage->code[0] = b;
		gte_stsxy3(&aGou->x0, &aGou->x1, &aGou->x2);
		gte_stlvnl2(&zed);

		OTPos = (uint *)((char *)pDoubleBuffer->pOT + (zed & 0x3ffc));
		addPrim(OTPos, aGou);
		addPrim(OTPos, ptpage);

		pGlass = (CGlassBit *)pGlass->mNext;
	}

	CGLine *pGLine;

	pGLine = GLineList;

	while (pGLine) {
		uint y0x01;
		uint y0x02;
		long zed;
		long zed2;

		Temp = (pGLine->mStart >> 12) - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();
		gte_stlvnl2(&zed);

		if (zed >= pCurrentViewport->Hither && zed <= pCurrentViewport->Yon) {
			gte_stsxy(&y0x01);

			Temp = (pGLine->mEnd >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();
			gte_stlvnl2(&zed2);

			if (zed2 >= pCurrentViewport->Hither && zed2 <= pCurrentViewport->Yon) {
				LINE_G2 *aline;
				uchar *newppoly;

				gte_stsxy(&y0x02);

				aline = (LINE_G2 *)pPoly;
				newppoly = pPoly + sizeof(LINE_G2);

				if (newppoly > PolyBufferEnd)
					return;

				pPoly = newppoly;
				aline->tag = 0x04000000;
				*(u_long *)&aline->r0 = pGLine->mCodeBGR0;
				*(u_long *)&aline->r1 = pGLine->mPadBGR1;
				*(u_long *)&aline->x0 = y0x01;
				*(u_long *)&aline->x1 = y0x02;

				if (zed2 < zed)
					zed = zed2;

				addPrim((uint *)((zed & 0x3ffc) + (uint)pDoubleBuffer->pOT), aline);
			}
		}

		pGLine = (CGLine *)pGLine->mNext;
	}

	CQuadBit *pQuad;

	pQuad = QuadBitList;

	while (pQuad) {
		if (pQuad->mpTexture) {
			long ZedB;
			long ZedC;
			long ZedD;
			long MaxZed;
			Texture *stored;
			POLY_FT4 *apoly;
			uchar *newppoly;
			uint foo1;
			uint foo2;
			uint foo3;

			Temp = (pQuad->mPos >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();

			stored = pQuad->mpTexture;
			apoly = (POLY_FT4 *)pPoly;
			newppoly = pPoly + sizeof(POLY_FT4);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			gte_stlvnl2(&MaxZed);

			if (MaxZed < pCurrentViewport->Hither || MaxZed > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			gte_stsxy(&apoly->x0);

			Temp = (pQuad->mPosB >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();
			gte_stlvnl2(&ZedB);

			if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			gte_stsxy(&apoly->x1);

			Temp = (pQuad->mPosC >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();

			apoly->tag = 0x09000000;
			*(u_long *)&apoly->r0 = pQuad->mCodeBGR;

			if (ZedB > MaxZed)
				MaxZed = ZedB;

			gte_stlvnl2(&ZedC);

			if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			gte_stsxy(&apoly->x2);

			Temp = (pQuad->mPosD >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();
			ushort foo4;

			foo1 = *(uint *)&stored->u0;
			foo2 = *(uint *)&stored->u1;
			foo3 = *(uint *)&stored->u2;
			foo4 = *(ushort *)&stored->u3;
			*(u_long *)&apoly->u0 = foo1;
			*(u_long *)&apoly->u1 = foo2;
			*(u_long *)&apoly->u2 = foo3;
			*(ushort *)&apoly->u3 = foo4;

			if (pQuad->mFlags & 4)
				apoly->tpage = (apoly->tpage & 0xff9f) | 0x40;

			if (ZedC > MaxZed)
				MaxZed = ZedC;

			gte_stlvnl2(&ZedD);

			if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			gte_stsxy(&apoly->x3);

			if (MaxZed < ZedD)
				MaxZed = ZedD;

			addPrim((uint *)(((MaxZed - pQuad->mFrigDeltaZ) & 0x3ffc) + (uint)pDoubleBuffer->pOT),
			        apoly);
		}
	NEXTQUAD:
		pQuad = (CQuadBit *)pQuad->mNext;
	}

	CChunkBit *p_chunk;

	p_chunk = ChunkBitList;

	while (p_chunk) {
		DR_TWIN *p_overflow2;
		long opz;
		long ZedA;
		long ZedB;
		long ZedC;
		long ZedD;
		long MaxZed;
		uint y0x0A;
		uint y0x0B;
		uint y0x0C;
		uint y0x0D;
		POLY_FT3 *aft3poly;
		DR_TWIN *pWinOff;
		DR_TWIN *pWinOn;
		uchar *newppoly;
		uint *pOTPos;

		if (pPoly + 152 > PolyBufferEnd)
			return;

		MaxZed = -1;
		Temp = p_chunk->mWorldPosA - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();
		gte_stlvnl2(&ZedA);
		gte_stsxy(&y0x0A);

		Temp = p_chunk->mWorldPosB - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();

		if (ZedA < pCurrentViewport->Hither || ZedA > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedA > MaxZed)
			MaxZed = ZedA;

		gte_stlvnl2(&ZedB);
		gte_stsxy(&y0x0B);

		Temp = p_chunk->mWorldPosC - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();

		if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedB > MaxZed)
			MaxZed = ZedB;

		gte_stlvnl2(&ZedC);
		gte_stsxy(&y0x0C);

		Temp = p_chunk->mWorldPosD - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();

		if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedC > MaxZed)
			MaxZed = ZedC;

		gte_stlvnl2(&ZedD);

		if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedD > MaxZed)
			MaxZed = ZedD;

		gte_stsxy(&y0x0D);

		pOTPos = (uint *)((char *)pDoubleBuffer->pOT + (MaxZed & 0x3ffc));

		if (p_chunk->mTexWin) {
			pWinOff = (DR_TWIN *)pPoly;
			pWinOn = pWinOff + 1;
			newppoly = (uchar *)(pWinOn + 1);
			pPoly = newppoly;

			{
				RECT win;

				win.x = win.y = win.w = win.h = 0;
				setTexWindow(pWinOff, &win);
				setTexWindow(pWinOn, &win);

				pWinOn->code[0] = p_chunk->mTexWin;
			}

			if (p_chunk->mTexWin)
				addPrim(pOTPos, pWinOff);
		}

		aft3poly = (POLY_FT3 *)pPoly;
		gte_ldsxy3(y0x0A, y0x0B, y0x0C);
		gte_nclip();
		gte_stopz(&opz);

		if (opz <= 0) {
			aft3poly->tag = 0x07000000;
			*(u_long *)&aft3poly->r0 = p_chunk->mB0G0R0 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0A;
			*(u_long *)&aft3poly->x1 = y0x0B;
			*(u_long *)&aft3poly->x2 = y0x0C;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		gte_ldsxy3(y0x0A, y0x0D, y0x0B);
		gte_nclip();
		gte_stopz(&opz);

		if (opz <= 0) {
			aft3poly->tag = 0x07000000;
			*(u_long *)&aft3poly->r0 = p_chunk->mB1G1R1 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0A;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0B;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		gte_ldsxy3(y0x0B, y0x0D, y0x0C);
		gte_nclip();
		gte_stopz(&opz);

		if (opz <= 0) {
			aft3poly->tag = 0x07000000;
			*(u_long *)&aft3poly->r0 = p_chunk->mB2G2R2 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0B;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0C;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		gte_ldsxy3(y0x0C, y0x0D, y0x0A);
		gte_nclip();
		gte_stopz(&opz);

		if (opz <= 0) {
			aft3poly->tag = 0x07000000;
			*(u_long *)&aft3poly->r0 = p_chunk->mB3G3R3 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0C;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0A;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		pPoly = (uchar *)aft3poly;

		if (p_chunk->mTexWin)
			addPrim(pOTPos, pWinOn);
	NEXTCHUNK:
		p_chunk = (CChunkBit *)p_chunk->mNext;
	}

	CPixel *pPixel;

	pPixel = PixelList;

	while (pPixel) {
		long zed;

		Temp = (pPixel->mPos >> 12) - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();

		if ((pPixel->mWidthHeight & 0xf) == 1) {
			TILE_1 *atile;
			uchar *newppoly;

			atile = (TILE_1 *)pPoly;
			newppoly = pPoly + sizeof(TILE_1);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			atile->tag = pPixel->tag;
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			gte_stlvnl2(&zed);

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			gte_stsxy(&atile->x0);
			addPrim((uint *)(((zed - pPixel->mFrigDeltaZ) & 0x3ffc) + (uint)pDoubleBuffer->pOT),
			        atile);
		} else {
			TILE *atile;
			uchar *newppoly;

			atile = (TILE *)pPoly;
			newppoly = pPoly + sizeof(TILE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			atile->tag = pPixel->tag;
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			*(u_long *)&atile->w = pPixel->mWidthHeight;
			gte_stlvnl2(&zed);

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			gte_stsxy(&atile->x0);
			addPrim((uint *)(((zed - pPixel->mFrigDeltaZ) & 0x3ffc) + (uint)pDoubleBuffer->pOT),
			        atile);
		}
	SKIPPIXEL:
		pPixel = (CPixel *)pPixel->mNext;
	}

	int Loop1x;
	int Loop1y;
	int Loop2x;
	int Loop2y;
	int StoredA1x;
	int StoredA1y;
	int StoredA2x;
	int StoredA2y;
	int StoredAx;
	int StoredAy;
	int Broken;
	CLinked2EndedBit *pLinked2EndedBit;

	Loop1x = 0;
	Loop1y = 0;
	Loop2x = 0;
	Loop2y = 0;
	StoredA1x = 0;
	StoredA1y = 0;
	StoredA2x = 0;
	StoredA2y = 0;
	StoredAx = 0;
	StoredAy = 0;
	Broken = 0;
	pLinked2EndedBit = Linked2EndedBitList;

	while (pLinked2EndedBit) {
		Texture *stored;
		long zed;
		uint Width;
		int dA;
		int Ax;
		int Ay;
		int NeedToTransformA;
		POLY_FT4 *apoly;
		uchar *newppoly;
		uint foo1;
		uint foo2;
		uint foo3;
		uint By0x0;
		int Bx;
		int By;
		int dB;
		int A1x;
		int A1y;
		int A2x;
		int A2y;
		int B1x;
		int B1y;
		int B2x;
		int B2y;
		int dy;
		int dx;
		int d;

		NeedToTransformA = (pLinked2EndedBit->mBitFlags & 16) || Broken;

		if (NeedToTransformA) {
			uint Ay0x0;

			Temp = (pLinked2EndedBit->mEndPosA >> 12) - camera_pos;
			gte_ldlv0(&Temp);
			gte_rtps();

			stored = pLinked2EndedBit->mpPSXFrame->pTexture;
			Width = pLinked2EndedBit->mpPSXFrame->Width;
			gte_stlvnl2(&zed);

			if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
				Broken = 1;
				goto NEXTL2E;
			}

			gte_stsxy(&Ay0x0);

			Ax = (short)Ay0x0;
			Ay = (short)(Ay0x0 >> 16);
			dA = (pLinked2EndedBit->mScale * Width) / zed >> 1;

			if (dA < 2)
				dA = 2;
		} else {
			stored = pLinked2EndedBit->mpPSXFrame->pTexture;
			Width = pLinked2EndedBit->mpPSXFrame->Width;
			Ax = StoredAx;
			Ay = StoredAy;
		}

		Temp = (pLinked2EndedBit->mEndPosB >> 12) - camera_pos;
		gte_ldlv0(&Temp);
		gte_rtps();

		apoly = (POLY_FT4 *)pPoly;
		newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		apoly->tag = 0x09000000;
		*(u_long *)&apoly->r0 = pLinked2EndedBit->mCodeBGR;
		ushort foo4;

		foo1 = *(uint *)&stored->u0;
		foo2 = *(uint *)&stored->u1;
		foo3 = *(uint *)&stored->u2;
		foo4 = *(ushort *)&stored->u3;
		*(u_long *)&apoly->u0 = foo1;
		*(u_long *)&apoly->u1 = foo2;
		*(u_long *)&apoly->u2 = foo3;
		*(ushort *)&apoly->u3 = foo4;
		apoly->u1--;
		apoly->v2--;
		apoly->u3--;
		apoly->v3--;
		gte_stlvnl2(&zed);

		if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
			pPoly = (uchar *)apoly;
			Broken = 1;
			goto NEXTL2E;
		}

		gte_stsxy(&By0x0);

		Bx = (short)By0x0;
		By = (short)(By0x0 >> 16);
		StoredAx = Bx;
		StoredAy = By;
		dB = (pLinked2EndedBit->mScale * Width) / zed >> 1;

		if (dB < 2)
			dB = 2;

		dy = By - Ay;
		dx = Bx - Ax;
		d = M3dMaths_SquareRoot0(dx * dx + dy * dy);

		if (!d) {
			pPoly = (uchar *)apoly;
			Broken = 1;
			goto NEXTL2E;
		}

		if (NeedToTransformA) {
			int dydA;
			int dxdA;

			dydA = dy * dA / d;
			dxdA = dx * dA / d;
			A1x = Ax + dydA;
			A1y = Ay - dxdA;
			A2x = Ax - dydA;
			A2y = Ay + dxdA;
		} else {
			A1x = StoredA1x;
			A1y = StoredA1y;
			A2x = StoredA2x;
			A2y = StoredA2y;
		}

		if (pLinked2EndedBit->mBitFlags & 16) {
			Loop1x = A1x;
			Loop1y = A1y;
			Loop2x = A2x;
			Loop2y = A2y;
		}

		if (pLinked2EndedBit->mBitFlags & 32) {
			B1x = Loop1x;
			B1y = Loop1y;
			B2x = Loop2x;
			B2y = Loop2y;
		} else {
			int dydB;
			int dxdB;

			dydB = dy * dB / d;
			dxdB = dx * dB / d;
			B1x = Bx + dydB;
			B1y = By - dxdB;
			B2x = Bx - dydB;
			B2y = By + dxdB;
		}

		StoredA1x = B1x;
		StoredA1y = B1y;
		StoredA2x = B2x;
		StoredA2y = B2y;

		if (pLinked2EndedBit->mBitFlags & 2) {
			if (!(pLinked2EndedBit->mBitFlags & 1)) {
				apoly->x0 = B2x;
				apoly->x1 = B1x;
				apoly->y0 = B2y;
				apoly->y1 = B1y;
				apoly->x2 = A2x;
				apoly->x3 = A1x;
				apoly->y2 = A2y;
				apoly->y3 = A1y;
			} else {
				apoly->x0 = B1x;
				apoly->x1 = B2x;
				apoly->y0 = B1y;
				apoly->y1 = B2y;
				apoly->x2 = A1x;
				apoly->x3 = A2x;
				apoly->y2 = A1y;
				apoly->y3 = A2y;
			}
		} else {
			if (pLinked2EndedBit->mBitFlags & 1) {
				apoly->x0 = A2x;
				apoly->x1 = B2x;
				apoly->y0 = A2y;
				apoly->y1 = B2y;
				apoly->x2 = A1x;
				apoly->x3 = B1x;
				apoly->y2 = A1y;
				apoly->y3 = B1y;
			} else {
				apoly->x0 = A1x;
				apoly->x1 = B1x;
				apoly->y0 = A1y;
				apoly->y1 = B1y;
				apoly->x2 = A2x;
				apoly->x3 = B2x;
				apoly->y2 = A2y;
				apoly->y3 = B2y;
			}
		}

		addPrim(
		    (uint *)(((zed - pLinked2EndedBit->mFrigDeltaZ) & 0x3ffc) + (uint)pDoubleBuffer->pOT),
		    apoly);

		Broken = 0;
	NEXTL2E:
		pLinked2EndedBit = (CLinked2EndedBit *)pLinked2EndedBit->mNext;
	}
}
