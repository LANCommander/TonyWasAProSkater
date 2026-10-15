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
#include "game/scratchpad.h"
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/bit.h"
#include "game/m3d.h"
#include "game/trig.h"
#include "game/camera.h"
#include "pc_draw.h"

extern SCamera MikeCamera[];
extern int Camera_CurrentMikeCamera;
extern SViewport *pCurrentViewport;
extern SDoubleBuffer *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;
extern SSinCos rcossin_tbl[];
extern ushort Xres;

/* Widescreen (platform/pc, pc_gpu_gl.h): a full-width view shows pc_wide_pad more columns on
   either side. */
extern "C" int pc_wide_pad;

extern CSpecialDisplay *SpecialDisplayList;
extern CGlow *GlowList;
extern CGPolyLine *GPolyLineList;
extern CPolyLine *PolyLineList;
extern CFlatBit *FlatBitList;
extern CGlassBit *GlassList;
extern CGLine *GLineList;
extern CQuadBit *QuadBitList;
extern CChunkBit *ChunkBitList;
extern CPixel *PixelList;
extern CLinked2EndedBit *Linked2EndedBitList;
extern DR_TPAGE StoredTPagePrim; /* bit/init.c */

int M3dAsm_ClipLine(SClipLineBuf *);
int M3dMaths_SquareRoot0(int);

/* Links a packet of n command words (the port's markers, pc_draw.h); 0 when the buffer is full. */
static int AddMark(P_TAG *OTPos, const u_long *words, int n) {
	P_TAG *p = (P_TAG *)pPoly;
	uchar *end = pPoly + sizeof(P_TAG) + n * sizeof(u_long);

	if (end > PolyBufferEnd)
		return 0;

	pPoly = end;
	setlen(p, n);

	for (int i = 0; i < n; i++)
		P_WORDS(p)[i] = words[i];

	addPrim(OTPos, p);
	return 1;
}

/* Also for CTexturedRibbon::Display. */
int AddFlatMark(P_TAG *OTPos, u_long code) {
	return AddMark(OTPos, &code, 1);
}

/* Packets made in screen space around projected points, linked at OTPos before this (and after a
   PC_DRAW_FLAT_END there): the view depths of their polygons' vertices 0-3 for the port
   (PC_DRAW_DEPTH), so that geometry in front hides them. Also for CTexturedRibbon::Display. */
int AddDepthMark(P_TAG *OTPos, const int z[4]) {
	u_long w[PC_DRAW_DEPTH_WORDS];

	w[0] = PC_DRAW_DEPTH;

	for (int i = 0; i < 4; i++)
		w[1 + i] = z[i] > 1 ? z[i] : 1;

	return AddMark(OTPos, w, PC_DRAW_DEPTH_WORDS);
}

/* A particle around one projection (zed, its view depth), ext pixels across at most: its depth
   brought forward by its size as if a ball, so that geometry it touches doesn't cut it. */
static int AddParticleDepth(P_TAG *OTPos, int zed, int ext) {
	int z = zed - ext * zed / pCurrentViewport->Zoom;
	int zs[4] = {z, z, z, z};

	return AddDepthMark(OTPos, zs);
}

static u_long FadeBGR(u_long bgr, int fade) {
	if (!fade)
		return bgr & 0xffffff;

	return (((bgr & 0xff) * fade >> 8) | (((bgr >> 8) & 0xff) * fade >> 8) << 8 |
	        (((bgr >> 16) & 0xff) * fade >> 8) << 16);
}

/* A plain glow (a light's: one fringe, all sections drawn) for the port to draw smoothly
   (PC_DRAW_GLOW, pc_draw.h); others are drawn as the PS1 had them. */
static int AddGlowMark(P_TAG *OTPos, CGlow *pGlow, int CentreX, int CentreY, int vz, int fade) {
	u_long w[PC_DRAW_GLOW_WORDS];
	uint i;

	if (pGlow->mNumSections != 8 || pGlow->mNumFringes != 1 || pGlow->mSkipTriangles ||
	    (pGlow->mMask & 0xff) != 0xff)
		return AddFlatMark(OTPos, PC_DRAW_FLAT_BEGIN);

	w[0] = PC_DRAW_GLOW;
	w[1] = ((u_long)(ushort)CentreY << 16) | (ushort)CentreX;
	w[2] = FadeBGR(pGlow->mCentreCodeBGR, fade);
	w[3] = FadeBGR(pGlow->mpSections[0].CodeBGR, fade);
	w[4] = FadeBGR(pGlow->mpFringes[0].CodeBGR, fade);
	w[5] = (pGlow->mAngle & 4095) | (pGlow->mStepAngle & 0xffff) << 16;

	/* screen radii (vertical, in 16ths of a pixel) as Bit_Display has them: Radius * 400 / z */
	{
		uint r = pGlow->mpFringes[0].Width * 400 * 16 / vz;

		w[6] = r > 0xffff ? 0xffff : r;
	}

	for (i = 0; i < 8; i += 2) {
		uint r0 = pGlow->mpSections[i].Radius * 400 * 16 / vz;
		uint r1 = pGlow->mpSections[i + 1].Radius * 400 * 16 / vz;

		w[7 + i / 2] = (r0 > 0xffff ? 0xffff : r0) | (r1 > 0xffff ? 0xffff : r1) << 16;
	}

	return AddMark(OTPos, w, PC_DRAW_GLOW_WORDS);
}

void Bit_Display() {
	CVector Temp;
	CVector camera_pos;
	int GTEZedLimit;
	CSpecialDisplay *pSpecial;
	CGlow *pGlow;

	GeomProj proj; /* the last projection's results */

	geom_ld_mat(pc_geom.rot, &MikeCamera[Camera_CurrentMikeCamera].View);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	camera_pos.vx = MikeCamera[Camera_CurrentMikeCamera].Position.vx;
	camera_pos.vy = MikeCamera[Camera_CurrentMikeCamera].Position.vy;
	camera_pos.vz = MikeCamera[Camera_CurrentMikeCamera].Position.vz;
	GTEZedLimit = pCurrentViewport->Zoom >> 1;

	/* the glows clamp their corners to 100 columns beyond the screen; in widescreen beyond its
	   margins, as far as the GPU's polygon width limit (1023) allows */
	int XMin = -100;
	int XMax = 612;

	if (pCurrentViewport->xL == 0 && pCurrentViewport->xR == Xres) {
		int pad = pc_wide_pad < 155 ? pc_wide_pad : 155;

		XMin -= pad;
		XMax += pad;
	}
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

			proj = geom_project_lv(&Centre);
			DR_TPAGE *ptpage = (DR_TPAGE *)pPoly;
			uchar *newppoly = pPoly + sizeof(DR_TPAGE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			uint a = getlen(&StoredTPagePrim);
			uint b = StoredTPagePrim.code[0];

			setlen(ptpage, a);
			ptpage->code[0] = b;
			int n = pGlow->mNumSections;
			int Step = pGlow->mStepAngle;
			int Ang = pGlow->mAngle;
			SSection *pSection = pGlow->mpSections;

			geom_st_lv(&RotCoords, proj.v.mac);

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
				geom_st_word(&y0x0, proj.sxy);

				CentreX = (short)y0x0;
				CentreY = (short)(y0x0 >> 16);
			}

			if (CentreX == -1024 || CentreX == 1023 || CentreY == -1024 || CentreY == 1023)
				continue;

			P_TAG *OTPos = pDoubleBuffer->pOT + ((RotCoords.vz & 0x3ffc) >> 2);

			/* the glow is made in screen space around its centre's projection: the port draws
			   it as given (linked first, so the end marker follows the glow's packets) */
			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			DVECTOR *pBufA = (DVECTOR *)SCRATCHPAD;
			DVECTOR *pBufB = (DVECTOR *)(SCRATCHPAD + 0x4) + pGlow->mNumFringes;
			SSinCos *pTab = rcossin_tbl;
			SSinCos *p = &pTab[Ang & 4095];
			int RadA = pSection[n - 1].Radius * 400 / RotCoords.vz;
			int MSin = p->s * 400 / 256;
			int MCos = p->c;
			int x = CentreX + RadA * MSin / 4096;
			int y = CentreY - RadA * MCos / 4096;

			if (x > XMax)
				x = XMax;

			if (x < XMin)
				x = XMin;

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

					if (x > XMax)
						x = XMax;

					if (x < XMin)
						x = XMin;

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
					SSinCos *q = (SSinCos *)((Ang & 4095) * 4 + (uintptr_t)pTab);

					MSin = q->s * 400 / 256;
					MCos = q->c;
					x = CentreX + RadB * MSin / 4096;
					y = CentreY - RadB * MCos / 4096;

					if (x > XMax)
						x = XMax;

					if (x < XMin)
						x = XMin;

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
							setlen(aGou, 6);
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

							if (x > XMax)
								x = XMax;

							if (x < XMin)
								x = XMin;

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
								setlen(aGou, 8);
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
			AddGlowMark(OTPos, pGlow, CentreX, CentreY, RotCoords.vz, RGBFade);
		}
	}

	CGPolyLine *pGPolyLine;

	{
		uint LastBGR;
		SLineSeg *pSeg;
		SClipLineBuf *pClip = (SClipLineBuf *)SCRATCHPAD;

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
					P_TAG *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_G2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_G2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					setlen(aline, 4);
					*(u_long *)&aline->r0 = LastBGR;
					*(u_long *)&aline->r1 = NewBGR;

					if (pGPolyLine->mForcedPriority)
						otpos = pDoubleBuffer->pOT + pGPolyLine->mForcedPriority;
					else
						otpos = pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (LastBGR & 0x02000000) {
						uint a;
						uint b;

						a = getlen(&StoredTPagePrim);
						b = StoredTPagePrim.code[0];
						setlen(ptpage, a);
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
		SClipLineBuf *pClip = (SClipLineBuf *)SCRATCHPAD;

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
					P_TAG *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_F2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_F2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					setlen(aline, 3);
					*(u_long *)&aline->r0 = CodeBGR;

					if (pPolyLine->mForcedPriority)
						otpos = pDoubleBuffer->pOT + pPolyLine->mForcedPriority;
					else
						otpos = pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (CodeBGR & 0x02000000) {
						uint a;
						uint b;

						a = getlen(&StoredTPagePrim);
						b = StoredTPagePrim.code[0];
						setlen(ptpage, a);
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
		int zed;
		uint y0x0;

		Temp.vx = (pFlatBit->mPos.vx >> 12) - camera_pos.vx;
		Temp.vy = (pFlatBit->mPos.vy >> 12) - camera_pos.vy;
		Temp.vz = (pFlatBit->mPos.vz >> 12) - camera_pos.vz;
		proj = geom_project_lv(&Temp);
		POLY_FT4 *apoly = (POLY_FT4 *)pPoly;
		uchar *newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(apoly, 9);
		*(u_long *)&apoly->r0 = pFlatBit->mCodeBGR;
		zed = proj.v.mac[2];

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

		geom_st_word(&y0x0, proj.sxy);
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

		{
			P_TAG *OTPos;

			if (pFlatBit->mForcedPriority)
				OTPos = pDoubleBuffer->pOT + pFlatBit->mForcedPriority;
			else
				OTPos = pDoubleBuffer->pOT + (((zed - pFlatBit->mFrigDeltaZ) & 0x3ffc) >> 2);

			/* the port: the sprite at its own depth (its corners are only near projections) */
			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, apoly);

			if (pFlatBit->mForcedPriority) {
				if (!AddFlatMark(OTPos, PC_DRAW_FLAT_BEGIN))
					return;
			} else {
				short xs[4] = {apoly->x0, apoly->x1, apoly->x2, apoly->x3};
				short ys[4] = {apoly->y0, apoly->y1, apoly->y2, apoly->y3};
				int x0 = xs[0], x1 = xs[0], y0 = ys[0], y1 = ys[0];

				for (int i = 1; i < 4; i++) {
					x0 = xs[i] < x0 ? xs[i] : x0;
					x1 = xs[i] > x1 ? xs[i] : x1;
					y0 = ys[i] < y0 ? ys[i] : y0;
					y1 = ys[i] > y1 ? ys[i] : y1;
				}

				if (!AddParticleDepth(OTPos, zed - pFlatBit->mFrigDeltaZ,
				                  x1 - x0 > y1 - y0 ? x1 - x0 : y1 - y0))
					return;
			}
		}
	}

	CGlassBit *pGlass;

	pGlass = GlassList;

	while (pGlass) {
		SVECTOR STemp;
		int zed;
		int16_t tri[3][3];
		GeomProj triProj[3];
		POLY_F3 *aGou;
		DR_TPAGE *ptpage;
		uchar *newppoly;
		uint a;
		uint b;
		P_TAG *OTPos;

		STemp.vx = (pGlass->mPosA.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosA.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosA.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[0], &STemp);

		STemp.vx = (pGlass->mPosB.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosB.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosB.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[1], &STemp);

		STemp.vx = (pGlass->mPosC.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosC.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosC.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[2], &STemp);
		geom_rtpt(&pc_geom, tri, 12, 0, triProj, 0, 0);

		aGou = (POLY_F3 *)pPoly;
		ptpage = (DR_TPAGE *)(aGou + 1);
		newppoly = pPoly + sizeof(POLY_F3) + sizeof(DR_TPAGE);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(aGou, 4);
		aGou->code = 0x22;
		aGou->r0 = pGlass->mR;
		aGou->g0 = pGlass->mG;
		aGou->b0 = pGlass->mB;
		a = getlen(&StoredTPagePrim);
		b = StoredTPagePrim.code[0];
		setlen(ptpage, a);
		ptpage->code[0] = b;
		geom_st_word(&aGou->x0, triProj[0].sxy);
		geom_st_word(&aGou->x1, triProj[1].sxy);
		geom_st_word(&aGou->x2, triProj[2].sxy);
		zed = triProj[2].v.mac[2];

		OTPos = pDoubleBuffer->pOT + ((zed & 0x3ffc) >> 2);
		addPrim(OTPos, aGou);
		addPrim(OTPos, ptpage);

		pGlass = (CGlassBit *)pGlass->mNext;
	}

	CGLine *pGLine;

	pGLine = GLineList;

	while (pGLine) {
		uint y0x01;
		uint y0x02;
		int zed;
		int zed2;

		Temp = (pGLine->mStart >> 12) - camera_pos;
		proj = geom_project_lv(&Temp);
		zed = proj.v.mac[2];

		if (zed >= pCurrentViewport->Hither && zed <= pCurrentViewport->Yon) {
			geom_st_word(&y0x01, proj.sxy);

			Temp = (pGLine->mEnd >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
			zed2 = proj.v.mac[2];

			if (zed2 >= pCurrentViewport->Hither && zed2 <= pCurrentViewport->Yon) {
				LINE_G2 *aline;
				uchar *newppoly;

				geom_st_word(&y0x02, proj.sxy);

				aline = (LINE_G2 *)pPoly;
				newppoly = pPoly + sizeof(LINE_G2);

				if (newppoly > PolyBufferEnd)
					return;

				pPoly = newppoly;
				setlen(aline, 4);
				*(u_long *)&aline->r0 = pGLine->mCodeBGR0;
				*(u_long *)&aline->r1 = pGLine->mPadBGR1;
				*(u_long *)&aline->x0 = y0x01;
				*(u_long *)&aline->x1 = y0x02;

				if (zed2 < zed)
					zed = zed2;

				addPrim(pDoubleBuffer->pOT + ((zed & 0x3ffc) >> 2), aline);
			}
		}

		pGLine = (CGLine *)pGLine->mNext;
	}

	CQuadBit *pQuad;

	pQuad = QuadBitList;

	while (pQuad) {
		if (pQuad->mpTexture) {
			int ZedB;
			int ZedC;
			int ZedD;
			int MaxZed;
			Texture *stored;
			POLY_FT4 *apoly;
			uchar *newppoly;
			uint foo1;
			uint foo2;
			uint foo3;

			Temp = (pQuad->mPos >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);

			stored = pQuad->mpTexture;
			apoly = (POLY_FT4 *)pPoly;
			newppoly = pPoly + sizeof(POLY_FT4);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			MaxZed = proj.v.mac[2];

			if (MaxZed < pCurrentViewport->Hither || MaxZed > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x0, proj.sxy);

			Temp = (pQuad->mPosB >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
			ZedB = proj.v.mac[2];

			if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x1, proj.sxy);

			Temp = (pQuad->mPosC >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);

			setlen(apoly, 9);
			*(u_long *)&apoly->r0 = pQuad->mCodeBGR;

			if (ZedB > MaxZed)
				MaxZed = ZedB;

			ZedC = proj.v.mac[2];

			if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x2, proj.sxy);

			Temp = (pQuad->mPosD >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
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

			ZedD = proj.v.mac[2];

			if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x3, proj.sxy);

			if (MaxZed < ZedD)
				MaxZed = ZedD;

			addPrim(pDoubleBuffer->pOT + (((MaxZed - pQuad->mFrigDeltaZ) & 0x3ffc) >> 2),
			        apoly);
		}
	NEXTQUAD:
		pQuad = (CQuadBit *)pQuad->mNext;
	}

	CChunkBit *p_chunk;

	p_chunk = ChunkBitList;

	while (p_chunk) {
		DR_TWIN *p_overflow2;
		int opz;
		int ZedA;
		int ZedB;
		int ZedC;
		int ZedD;
		int MaxZed;
		uint y0x0A;
		uint y0x0B;
		uint y0x0C;
		uint y0x0D;
		POLY_FT3 *aft3poly;
		DR_TWIN *pWinOff;
		DR_TWIN *pWinOn;
		uchar *newppoly;
		P_TAG *pOTPos;

		if (pPoly + 152 > PolyBufferEnd)
			return;

		MaxZed = -1;
		Temp = p_chunk->mWorldPosA - camera_pos;
		proj = geom_project_lv(&Temp);
		ZedA = proj.v.mac[2];
		geom_st_word(&y0x0A, proj.sxy);

		Temp = p_chunk->mWorldPosB - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedA < pCurrentViewport->Hither || ZedA > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedA > MaxZed)
			MaxZed = ZedA;

		ZedB = proj.v.mac[2];
		geom_st_word(&y0x0B, proj.sxy);

		Temp = p_chunk->mWorldPosC - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedB > MaxZed)
			MaxZed = ZedB;

		ZedC = proj.v.mac[2];
		geom_st_word(&y0x0C, proj.sxy);

		Temp = p_chunk->mWorldPosD - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedC > MaxZed)
			MaxZed = ZedC;

		ZedD = proj.v.mac[2];

		if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedD > MaxZed)
			MaxZed = ZedD;

		geom_st_word(&y0x0D, proj.sxy);

		pOTPos = pDoubleBuffer->pOT + ((MaxZed & 0x3ffc) >> 2);

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
		opz = geom_nclip(y0x0A, y0x0B, y0x0C, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
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

		opz = geom_nclip(y0x0A, y0x0D, y0x0B, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
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

		opz = geom_nclip(y0x0B, y0x0D, y0x0C, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
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

		opz = geom_nclip(y0x0C, y0x0D, y0x0A, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
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
		int zed;
		P_TAG *OTPos; /* the port's depth marks */

		Temp = (pPixel->mPos >> 12) - camera_pos;
		proj = geom_project_lv(&Temp);

		if ((pPixel->mWidthHeight & 0xf) == 1) {
			TILE_1 *atile;
			uchar *newppoly;

			atile = (TILE_1 *)pPoly;
			newppoly = pPoly + sizeof(TILE_1);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			setlen(atile, pPixel->tag >> 24);
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			geom_st_word(&atile->x0, proj.sxy);
			OTPos = pDoubleBuffer->pOT + (((zed - pPixel->mFrigDeltaZ) & 0x3ffc) >> 2);

			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, atile);

			if (!AddParticleDepth(OTPos, zed - pPixel->mFrigDeltaZ, 1))
				return;
		} else {
			TILE *atile;
			uchar *newppoly;

			atile = (TILE *)pPoly;
			newppoly = pPoly + sizeof(TILE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			setlen(atile, pPixel->tag >> 24);
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			*(u_long *)&atile->w = pPixel->mWidthHeight;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			geom_st_word(&atile->x0, proj.sxy);
			OTPos = pDoubleBuffer->pOT + (((zed - pPixel->mFrigDeltaZ) & 0x3ffc) >> 2);

			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, atile);

			if (!AddParticleDepth(OTPos, zed - pPixel->mFrigDeltaZ,
			                  atile->w > atile->h ? atile->w : atile->h))
				return;
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
		int zed;
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
			proj = geom_project_lv(&Temp);

			stored = pLinked2EndedBit->mpPSXFrame->pTexture;
			Width = pLinked2EndedBit->mpPSXFrame->Width;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
				Broken = 1;
				goto NEXTL2E;
			}

			geom_st_word(&Ay0x0, proj.sxy);

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
		proj = geom_project_lv(&Temp);

		apoly = (POLY_FT4 *)pPoly;
		newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(apoly, 9);
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
		zed = proj.v.mac[2];

		if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
			pPoly = (uchar *)apoly;
			Broken = 1;
			goto NEXTL2E;
		}

		geom_st_word(&By0x0, proj.sxy);

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
		    pDoubleBuffer->pOT + (((zed - pLinked2EndedBit->mFrigDeltaZ) & 0x3ffc) >> 2),
		    apoly);

		Broken = 0;
	NEXTL2E:
		pLinked2EndedBit = (CLinked2EndedBit *)pLinked2EndedBit->mNext;
	}
}
