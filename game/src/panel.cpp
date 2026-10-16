/* PANEL.CPP: the on-screen panel (score, balance meter, time, trick names, SKATE letters and
   tapes, falling text) and its trick scoring, CPanelData (declarations in game/panel.h). */
#include "game/panel.h"
#include "game/dbuffer.h"
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "game/m3d.h"
#include "game/bruce.h"
#include "game/career.h"
#include "game/mess.h"
#include "game/shellutils.h"
#include "game/psyq.h"
#include "game/video.h"
#include "game/front.h"
#include "game/sfx.h"
#include "game/bit.h"
#include "game/globals.h"
#include "game/main.h"
#include "game/physics.h"
#include "game/powerup.h"
#include "game/spool.h"
#include "game/tricktips.h"
#include "game/utils.h"

extern int prim_added;
extern SAnimFrame *DAT_800d1e4c;
extern int TAPES_Display;
extern int TAPES_Pos;
extern int SKATE_Display;
extern int SKATE_Pos;
struct CBruce;
POLY_G4 *DrawGouraudPoly(int, int, int, int, uchar, uchar, uchar, uchar, uchar, uchar, int);
extern unsigned LastHoldingScore;
extern int Urgent;
void Panel_GetViewportMessOffset(int, short *, short *);
extern int Cheat_PanelOn;
extern int TAPES_Display, SKATE_Display;
void DisplayTapesSkate();
extern "C" void FUN_8001e100(FallingTextEffect *, int);
void DisplayScore(CBruce *, ushort, ushort);
extern "C" void FUN_8006d0c8(CBruce *, int, int);
void DisplayTime(CBruce *, ushort, ushort);
void DisplayOutOfBounds(CBruce *, ushort, ushort);
void Panel_ReplayInstructions();
extern "C" void FUN_8006e7fc(CPanelData *p, int v);
extern "C" void FUN_8006ea4c(CPanelData *p);
struct SMessageProg;
extern SMessageProg Messprog_Goal;
extern char DAT_800d3868[];
extern char Msg[];
extern int DAT_800d1ec4;

/* Panel_addPrim__FPv (0x8006c478) */
/* Adds a panel primitive to the 2D entry of the buffer being built. */
void Panel_addPrim(void *p) {
	addPrim(&pDoubleBuffer->OT2D, p);
	prim_added = 1;
}

/* FUN_8006c4c4 (0x8006c4c4) */
void FUN_8006c4c4(void) {
	DAT_800d1e4c = Spool_FindAnim("Font");
}

/* Panel_Update__Fv (0x8006c4e8) */
void Panel_Update(void) {}

/* Panel_DrawTexturedPoly__FP10SAnimFrame (0x8006c4f0) */
POLY_FT4 *Panel_DrawTexturedPoly(SAnimFrame *t) {
	POLY_FT4 *p = (POLY_FT4 *)pPoly;
	u_char *next = (u_char *)(p + 1);
	Texture *uv;

	if (next > PolyBufferEnd)
		return 0;

	pPoly = next;
	setlen(p, 9);
	*(u_long *)&p->r0 = 0x2c808080;
	uv = t->pTexture;

	{
		u_long a = *(u_long *)&uv->u0;
		u_long b = *(u_long *)&uv->u1;
		u_long c = *(u_long *)&uv->u2;
		u_short d = *(u_short *)&uv->u3;

		*(u_long *)&p->u0 = a;
		*(u_long *)&p->u1 = b;
		*(u_long *)&p->u2 = c;
		*(u_short *)&p->u3 = d;
	}

	addPrim(&pDoubleBuffer->OT2D, p);

	return p;
}

/* Panel_SetStretchedScreenCoords__FiiP8POLY_FT4P10SAnimFrame (0x8006c594) */
void Panel_SetStretchedScreenCoords(int x, int y, POLY_FT4 *p, SAnimFrame *f) {
	int x0 = x + (f->OffX << 9) / 320;
	int x1 = x0 + (f->Width << 9) / 320;
	int y0 = y + f->OffY;
	int y1 = y0 + f->Height;

	p->y0 = y0;
	p->y1 = y0;
	p->y2 = y1;
	p->y3 = y1;
	p->x0 = x0;
	p->x1 = x1;
	p->x2 = x0;
	p->x3 = x1;
}

/* Panel_DrawFlatShadedPoly__FiiiiUcUcUci (0x8006c610) */
/* Demo twin; locals from its MAIN.SYM. Adds a flat-shaded rectangle to the
   ordering table at the given depth; returns the primitive, or 0 if the primitive buffer is full. */
POLY_F4 *Panel_DrawFlatShadedPoly(int x, int y, int w, int h, uchar r, uchar g, uchar b, int sort) {
	POLY_F4 *apoly = (POLY_F4 *)pPoly;
	uchar *newppoly = pPoly + sizeof(POLY_F4);

	if (newppoly > PolyBufferEnd)
		return 0;

	pPoly = newppoly;
	setPolyF4(apoly);
	setRGB0(apoly, r, g, b);
	setXY4(apoly, x, y, x + w, y, x, y + h, x + w, y + h);
	addPrim(&(&pDoubleBuffer->OT2D)[sort], apoly);
	return apoly;
}

/* DrawGouraudPoly__FiiiiUcUcUcUcUcUci (0x8006c6e4) */
/* Demo twin; locals from its MAIN.SYM. Adds a Gouraud-shaded rectangle (left
   edge in the bottom colour, right edge in the top colour) to the ordering table at the given
   depth; returns the primitive, or 0 if the primitive buffer is full. */
POLY_G4 *DrawGouraudPoly(int x, int y, int w, int h, uchar rtop, uchar gtop, uchar btop,
                         uchar rbottom, uchar gbottom, uchar bbottom, int sort) {
	POLY_G4 *apoly = (POLY_G4 *)pPoly;
	uchar *newppoly = pPoly + sizeof(POLY_G4);

	if (newppoly > PolyBufferEnd)
		return 0;

	pPoly = newppoly;
	setPolyG4(apoly);

	apoly->r0 = rbottom;
	apoly->r1 = rtop;
	apoly->g0 = gbottom;
	apoly->g1 = gtop;
	apoly->b0 = bbottom;
	apoly->b1 = btop;
	apoly->r2 = rbottom;
	apoly->r3 = rtop;
	apoly->g2 = gbottom;
	apoly->g3 = gtop;
	apoly->b2 = bbottom;
	apoly->b3 = btop;
	apoly->x0 = x;
	apoly->x1 = x + w;
	apoly->y0 = y;
	apoly->y1 = y;
	apoly->x2 = x;
	apoly->x3 = x + w;
	apoly->y2 = y + h;
	apoly->y3 = y + h;
	addPrim(&(&pDoubleBuffer->OT2D)[sort], apoly);
	return apoly;
}

/* DisplayTapesSkate__Fv (0x8006c7e8) */
void DisplayTapesSkate(void) {
	CBruce *pSkater = MechList;

	if (!pSkater->mNextItem) {
		if (SKATE_Display) {
			if (SKATE_Pos >= 483 && SKATE_Display > 10)
				SKATE_Pos -= 4;

			if (SKATE_Display < 10)
				SKATE_Pos = 522 - SKATE_Display * 4;

			SKATE_Display--;
			Mess_SetRGB(((Xblanks % 20) * 12) & 252, 100, 100, 0);
			Mess_SetTextJustify(1);
			Mess_SetScale(255);

			if (pSkater->mPanel.SKATE[0])
				Mess_DrawPanelText(SKATE_Pos, 80, "S", 0, 0, 256);

			if (pSkater->mPanel.SKATE[1])
				Mess_DrawPanelText(SKATE_Pos, 105, "K", 0, 0, 256);

			if (pSkater->mPanel.SKATE[2])
				Mess_DrawPanelText(SKATE_Pos, 130, "A", 0, 0, 256);

			if (pSkater->mPanel.SKATE[3])
				Mess_DrawPanelText(SKATE_Pos, 155, "T", 0, 0, 256);

			if (pSkater->mPanel.SKATE[4])
				Mess_DrawPanelText(SKATE_Pos, 180, "E", 0, 0, 256);
		}

		if (TAPES_Display) {
			if (TAPES_Pos >= 482 && TAPES_Display > 10)
				TAPES_Pos -= 4;

			if (TAPES_Display < 11)
				TAPES_Pos = 511 - TAPES_Display * 4;

			TAPES_Display--;
		}
	}
}

/* GraffitiColor__FP6CBruce (0x8006ca1c) */
void GraffitiColor(struct CBruce *a) {
	if (GGame == 8) {
		if (a == GSkater[0])
			Mess_SetRGB(80, 80, 128, 0);
		else
			Mess_SetRGB(128, 80, 80, 0);
	}
}

/* DisplayScore__FP6CBruceUsUs (0x8006ca78) */
/* From the demo twin + its MAIN.SYM locals (DoDisplay; block r/g/b; block score[15]
   with inner Value). Draws the balance meter and score panel, counts ScoreToAdd into Score, awards
   the two score tapes in career mode, and prints the score (flashing above 999999).
   Levers: DrawGouraudPoly returns POLY_G4* and Mess_DrawPanelText returns int (register choice of
   the stacked args); GHorseLetters is a small sized extern ([2]) so its address is one `la` and the
   Horse_DisplayWords jal delay slot stays empty; Sprite::draw takes ints (no &255 on the flash). */
void DisplayScore(CBruce *pBruce, ushort xpos, ushort ypos) {
	/* its TU saw Xblanks as volatile */
	volatile uint &VXblanks = Xblanks;
	int DoDisplay = GVInfo1[0].VideoMode != 2;

	if (Cheat_Fire) {
		pBruce->mBalanceFlash = 750;
		pBruce->mBalanceTime = 750;
	}

	if (GGame == 7) {
		if (!Cheat_Fire) {
			pBruce->mBalanceFlash = 0;
			pBruce->mBalanceTime = -1;
		}

		Horse_DisplayWords(GHorseLetters);
		return;
	}

	Mess_SetCurrentFont(DAT_800d1798);

	if (DoDisplay) {
		{
			int r;
			int g;
			int b;

			if (pBruce->mBalanceTime != -1) {
				if (pBruce->mBalanceFlash)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->mBalanceFlash * 120 / 750, 3,
					                (VXblanks % 40) * 6, (VXblanks % 40) * 6, 50, (VXblanks % 40) * 6,
					                (VXblanks % 40) * 6, 50, 0);
				else if (pBruce->mBalanceTime)
					DrawGouraudPoly(xpos - 1, ypos + 10, pBruce->mBalanceTime / 25, 3, 0, 150, 0, 0,
					                150, 50, 0);
			}

			OT_entry = 1;
			r = -1;
			g = -1;
			b = -1;

			if (GGame == 8) {
				if (pBruce == GSkater[0]) {
					r = 80;
					g = 80;
					b = 128;
				} else {
					r = 128;
					g = 80;
					b = 80;
				}
			}

			pScoreSprite->draw(xpos, ypos - 20, 1, r, g, b);

			if (GGame != 7) {
				if (pBruce->mBalanceFlash == 0)
					pBarSprite->draw(xpos - 4, ypos, 1, r, g, b);
				else
					pBarSprite->draw(xpos - 4, ypos, 1, (VXblanks % 40) * 6, (VXblanks % 40) * 6, 50);
			}
		}
	}

	if (GGame != 7 && pBruce->mPanel.mLandedTotal && !pBruce->mPanel.m4F4) {
		if (GGame == 8) {
			pBruce->mPanel.mLandedTotal = 0;
		} else if (pBruce->mPanel.mLandedTotal > 10000) {
			pBruce->mPanel.mTotalScore += 250;
			pBruce->mPanel.mLandedTotal -= 250;
		} else if (pBruce->mPanel.mLandedTotal > 2000) {
			pBruce->mPanel.mTotalScore += 100;
			pBruce->mPanel.mLandedTotal -= 100;
		} else if (pBruce->mPanel.mLandedTotal > 25) {
			pBruce->mPanel.mTotalScore += 25;
			pBruce->mPanel.mLandedTotal -= 25;
		} else {
			pBruce->mPanel.mTotalScore += pBruce->mPanel.mLandedTotal;
			pBruce->mPanel.mLandedTotal = 0;
		}
	}

	if (DoDisplay && GGame == 1 && !Levels[GLevel].isCompetition) {
		if (pBruce->mPanel.mTotalScore >= Levels[GLevel].HiScore && !Career_Got(0))
			Panel_Tape(0);

		if (pBruce->mPanel.mTotalScore >= Levels[GLevel].ProScore && !Career_Got(4))
			Panel_Tape(4);
	}

	if (DoDisplay) {
		char score[15];

		if (pBruce->mPanel.mTotalScore > 999999) {
			sprintf(score, "Yowza!", pBruce->mPanel.mTotalScore);
			Mess_SetRGB((VXblanks % 20) * 12, 100, 100, 0);
		} else {
			Mess_SetRGB(128, 128, 128, 0);

			{
				int Value = pBruce->mPanel.mTotalScore;

				if (GGame == 8) {
					Value = pBruce->mGraffitiCount;
					GraffitiColor(pBruce);
				}

				sprintf(score, "%ld", (long)Value);
			}
		}

		Mess_SetTextJustify(2);
		Mess_SetScale(256);
		Mess_DrawPanelText(xpos + 125, ypos - 4, score, 0, 0, 256);
	}

	Mess_SetCurrentFont(GStartTime);
}

/* FUN_8006d0c8 (0x8006d0c8) */
/* Panel: the current combo/score readout (PANEL.CPP). Sums the pending trick scores, works out
   the multiplier, and draws the score and x-multiplier text. */
extern "C" void FUN_8006d0c8(CBruce *o, int x0, int y0) {
	char b1[16];
	char b2[16];
	unsigned s2;
	int n2, s3, x, y, s5, s4, n, k, i, v, r;
	int *p;

	s2 = 0;
	s3 = 0;
	x = x0 & 0xffff;
	y = y0 & 0xffff;
	s5 = GVInfo1[0].VideoMode != 2;

	if (GCareer.Options.ScoreDisplayEnabled) {
		n = o->mPanel.mTQStack;

		if (n > 0) {
			k = o->mNum180Spins;
			n2 = n * 2;

			if (k < 3)
				v = n2 + k;
			else
				v = n2 + (k * 2 - 2);

			s3 = v;

			if (o->mPhysicsState == 2 && k == 1)
				s3 -= 1;

			for (i = 0; i < o->mPanel.mTQStack; i++)
				s2 += o->mPanel.mTQ[i].point_value;
		}

		if (o->mPanel.mLandedTotal)
			s2 = o->mPanel.mLandedTotal;

		if (s2 || o->mPanel.mTrickState == 5) {
			sprintf(b1, "%ld", (long)(o->mPanel.mTrickState == 5 ? LastHoldingScore : s2));
			Mess_SetRGB(128, 128, 128, 0);
			GraffitiColor(o);

			s4 = s3 < 3;

			if (!s4)
				Mess_SetTextJustify(2);
			else
				Mess_SetTextJustify(0);

			Mess_SetScale(256);

			if (s5) {
				o->mPanel.mFallingTextEffect.bind(o->mPanel.mTrickState == 5);
				Mess_DrawPanelText(x, y, b1, 0, 0, 256);
				o->mPanel.mFallingTextEffect.bind(0);
			}

			if (o->mPanel.m4F4 > 0)
				o->mPanel.m4F4 = o->mPanel.m4F4 - 1;

			if (!s4) {
				if (s3 & 1)
					sprintf(b2, " x%d.5", s3 >> 1);
				else
					sprintf(b2, " x%d", s3 >> 1);

				Mess_SetRGB(128, 128, 128, 0);
				GraffitiColor(o);
				Mess_SetTextJustify(1);
				Mess_SetScale(256);

				if (s5)
					Mess_DrawPanelText(x, y, b2, 0, 0, 256);
			}

			if (s2)
				LastHoldingScore = (s2 * s3) >> 1;
		}

		if (o->mPanel.mTQStack) {
			int kk = o->mNum180Spins;

			if (o->mPanel.old_180spins < kk) {
				if (o->mPhysicsState != 2 || kk >= 2) {
					o->mPanel.TrickMessage();

					o->mPanel.old_180spins = o->mNum180Spins;
				}
			}
		}
	}
}

/* DisplayTime__FP6CBruceUsUs (0x8006d39c) */
/* Fuzzy demo twin (0.31 by opcode, the same shape with the Mess_ text calls);
   locals from its MAIN.SYM. Draws a skater's run clock as m:ss, flashing in the last seconds, and
   ticks the urgent beep (faster as time runs out) for the first player. The two SVideoInfo blocks
   are one extern array: an unsized extern gives the split lui/addiu that keeps the if/else (a
   single `la` lets jump.c turn it into `VInfo = B; if (...) VInfo = A`). */
void DisplayTime(CBruce *pBruce, ushort xpos, ushort ypos) {
	char time[8];
	int ticks;
	int mins;
	int secs;

	if (GGame == 2)
		return;

	if (GGame == 7 && !pBruce->mTime)
		return;

	Mess_SetCurrentFont(GSkaterRegion);

	{
		SVideoInfo *VInfo;

		if (pBruce == MechList)
			VInfo = &GVInfo1[0];
		else
			VInfo = &GVInfo1[1];

		if (VInfo->VideoMode != 2) {
			ticks = pBruce->mTime - XFrames;
			mins = ticks / 3600;
			secs = (ticks - mins * 3600) / 60;

			if (ticks < 0) {
				secs = 0;
				mins = 0;
			}

			sprintf(time, "%ld:%02ld", (long)mins, (long)secs);
			Mess_SetRGB(128, 0, 0, 0);
			Mess_SetTextJustify(0);
			Mess_SetScale(256);

			if (GGame == 7) {
				if (mins || secs >= 4 || Xblanks % 30 >= 15)
					Mess_DrawPanelText(xpos, ypos, time, 0, 0, 256);
			} else {
				if (mins || secs >= 11 || Xblanks % 60 >= 30)
					Mess_DrawPanelText(xpos, ypos, time, 0, 0, 256);

				if (pBruce == GSkater[0]) {
					if (ticks >= 1 && ticks < 600) {
						if (ticks < Urgent) {
							int sub;

							SFX_PlayX(SND_TIMEOUT, 8192, 0);

							sub = Urgent / 10;

							if (sub < 10)
								sub = 10;

							Urgent -= sub;
						}
					} else {
						Urgent = 600;
					}
				}
			}
		}
	}

	Mess_SetCurrentFont(GStartTime);
}

/* Panel_GetViewportMessOffset__FiPsT1 (0x8006d67c) */
void Panel_GetViewportMessOffset(int m, short *x, short *y) {
	switch (m) {
		case 0:
		case 1:
			*x = 0;
			*y = 0;
			break;
		case 4:
			*x = 0;
			*y = 60;
			break;
		case 6:
			*x = 128;
			*y = 60;
			break;
		case 2:
			*x = 128;
			*y = 0;
			break;
		case 5:
			*x = 0;
			*y = -60;
			break;
		case 7:
			*x = -128;
			*y = -60;
			break;
		case 3:
			*x = -128;
			*y = 0;
			break;
	}
}

/* DisplayOutOfBounds__FP6CBruceUsUs (0x8006d704) */
/* PANEL.CPP (demo twin; its pString local). Shows the out-of-bounds message (one of four by
   reason) as a text effect at the player's viewport offset until the effect completes. */
void DisplayOutOfBounds(CBruce *p, ushort a, ushort b) {
	short x, y;
	char *s0;

	if (p->mPanel.mOutOfBoundsMessage) {
		Panel_GetViewportMessOffset(ViewportMode * 2 + p->mPanel.mPlayerNum, &x, &y);

		switch (p->mPanel.mOutOfBoundsMessage) {
			case 1:
				s0 = GameText.pOutOfBounds1;
				break;
			case 2:
				s0 = GameText.pOutOfBounds2;
				break;
			case 3:
				s0 = GameText.pOutOfBounds3;
				break;
			default:
				s0 = GameText.pOutOfBounds4;
				break;
		}

		p->mPanel.mOutOfBoundsText.update();
		p->mPanel.mOutOfBoundsText.setScale(256);
		p->mPanel.mOutOfBoundsText.bind(1);
		Mess_SetTextJustify(0);
		Mess_DrawPanelText(a + x, b + y, s0, 0, 0, 256);

		if (p->mPanel.mOutOfBoundsText.handleComplete() == 0)
			p->mPanel.mOutOfBoundsMessage = 0;

		p->mPanel.mOutOfBoundsText.bind(0);
	}
}

/* Panel_ReplayInstructions__Fv (0x8006d858) */
void Panel_ReplayInstructions(void) {
	if (GVInfo1[0].VideoMode == 2 && DemoTimer && Status != 20 && Status != 22 && Status != 21) {
		int m = GVInfo1[0].ReplayHighlight;

		if ((m == 0 && (unsigned)(GVInfo1[0].NumFrames - GVInfo1[0].FramesLeft) < 90) || m == 1) {
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetScale(188);
			Mess_SetTextJustify(0);
			Mess_DrawPanelText(256, 220, GameText.pReplayInstructions, 0, 0, 256);
		}
	}
}

/* DisplayTrick__10CPanelDataP6CBruceUsUs (0x8006d938) */
/* Demo twin CPanelData::DisplayTrick by signature and prologue (the demo body
   uses the Font/TextEffect classes THPS1 lacks). Draws the three trick-name lines bottom-up and
   animates the text scale: grow from 60 (state 1), settle to normal (2), hold for 30 frames (3),
   shrink away (4), or hand over to the message effect at +0x164 (5). Called on pBruce->mPanel.
   The per-mode scale pair is set in both arms (NormalScale first): that keeps jump.c from
   rewriting the if/else into "x = b; if (c) x = a;", giving retail's bne; j shape. */
void CPanelData::DisplayTrick(CBruce *pBruce, ushort xpos, ushort ypos) {
	int ok = GVInfo1[0].VideoMode != 2;

	if (!ok)
		return;

	if (!GCareer.Options.ScoreDisplayEnabled)
		return;

	Mess_SetRGB(128, 128, 128, 0);
	GraffitiColor(pBruce);
	Mess_SetTextJustify(0);
	int NormalScale;
	int BigScale;

	if (GNumberOfPlayers == 1) {
		NormalScale = 188;
		BigScale = 256;
	} else {
		NormalScale = 188;
		BigScale = 188;
	}

	switch (mTrickState) {
		case 0:
			return;
		case 1: {
			int Frames = 5;

			Mess_SetScale((BigScale - 60) * mTrickTimer / Frames + 60);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 2;
			}

			break;
		}
		case 2: {
			int Frames = 5;

			Mess_SetScale(BigScale + (NormalScale - BigScale) * mTrickTimer / Frames);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 3;
			}

			break;
		}
		case 3:
			Mess_SetScale(188);

			if (mTrickTimer == 30) {
				mTrickTimer = 0;
				mTrickState = 4;
			}

			if (mLastMult) {
				mTrickState = 5;
				mLastMult = 0;
			}

			break;
		case 4: {
			int Frames = 10;

			Mess_SetScale(NormalScale + (0 - NormalScale) * mTrickTimer / Frames);

			if (mTrickTimer == Frames) {
				mTrickTimer = 0;
				mTrickState = 0;
			}

			if (mLastMult)
				mLastMult = 0;

			break;
		}
		case 5:
			break;
	}

	if (mTrickState == 5)
		pBruce->mPanel.mFallingTextEffect.bind(1);
	else
		pBruce->mPanel.mFallingTextEffect.setScale(Mess_GetScale());

	if (!GameFrozen)
		mTrickTimer++;

	int LineHeight = Mess_GetScale() * 13 / 188;

	if (pBruce->mPanel.mTrickName3[0]) {
		Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName3, 0, 0, 256);

		ypos -= LineHeight;
	}

	if (pBruce->mPanel.mTrickName2[0]) {
		Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName2, 0, 0, 256);

		ypos -= LineHeight;
	}

	Mess_DrawPanelText(xpos, ypos, pBruce->mPanel.mTrickName, 0, 0, 256);
	int Busy = pBruce->mPanel.mFallingTextEffect.handleComplete();

	if (mTrickState == 5) {
		pBruce->mPanel.mFallingTextEffect.bind(0);

		if (!Busy)
			pBruce->mPanel.mTrickState = 0;
	}
}

/* FUN_8006dc78 (0x8006dc78) */
extern "C" void FUN_8006dc78() {
	CBruce *s2;
	FallingTextEffect *s3;

	if (Cheat_PanelOn) {
		s2 = MechList;
		Panel_ReplayInstructions();

		if (s2) {
			s3 = &s2->mPanel.mFallingTextEffect;

			do {
				s3->update();
				DisplayOutOfBounds(s2, 256, 90);
				DisplayTapesSkate();

				if (GamePaused) {
					SKATE_Display = 30;
					TAPES_Display = 30;
					return;
				}

				switch (ViewportMode * 2 + s2->mPanel.mPlayerNum) {
					case 0:
					case 1:
						TrickTips_Display();
						FUN_8006d0c8(s2, 256, 224);
						s2->mPanel.DisplayTrick(s2, 256, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 2:
						FUN_8006d0c8(s2, 384, 224);
						s2->mPanel.DisplayTrick(s2, 384, 205);
						DisplayScore(s2, 358, 45);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 4:
						s2->mPanel.DisplayTrick(s2, 256, 205);
						FUN_8006d0c8(s2, 256, 224);
						DisplayScore(s2, 25, 145);
						DisplayTime(s2, 452, 150);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 6:
						FUN_8006d0c8(s2, 128, 224);
						s2->mPanel.DisplayTrick(s2, 128, 205);
						DisplayScore(s2, 112, 150);
						DisplayTime(s2, 55, 155);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						Panel_DrawFlatShadedPoly(256, 0, 256, 120, 30, 30, 30, 1);
						break;
					case 8:
						FUN_8006d0c8(s2, 256, 224);
						s2->mPanel.DisplayTrick(s2, 256, 205);
						DisplayScore(s2, 70, 175);
						DisplayTime(s2, 133, 217);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 3:
						FUN_8006d0c8(s2, 128, 224);
						s2->mPanel.DisplayTrick(s2, 128, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 240);
						break;
					case 5:
						s2->mPanel.DisplayTrick(s2, 256, 85);
						FUN_8006d0c8(s2, 256, 104);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 452, 50);
						FUN_8001e100(s3, 118);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 118);
						break;
					case 7:
						FUN_8006d0c8(s2, 384, 104);
						s2->mPanel.DisplayTrick(s2, 384, 85);
						DisplayScore(s2, 276, 45);
						DisplayTime(s2, 457, 50);
						Panel_DrawFlatShadedPoly(0, 120, 257, 2, 30, 30, 30, 1);
						Panel_DrawFlatShadedPoly(0, 120, 257, 120, 20, 20, 20, 1);
						Panel_DrawFlatShadedPoly(256, 119, 512, 2, 30, 30, 30, 1);
						FUN_8001e100(s3, 122);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 122);
						break;
					case 9:
						FUN_8006d0c8(s2, 256, 154);
						s2->mPanel.DisplayTrick(s2, 256, 130);
						DisplayScore(s2, 315, 43);
						DisplayTime(s2, 371, 85);
						FUN_8001e100(s3, 152);
						FUN_8001e100(&s2->mPanel.mOutOfBoundsText, 152);
						break;
				}

				s2 = (CBruce *)s2->mNextItem;
				s3 = &s2->mPanel.mFallingTextEffect;
			} while (s2);
		}
	}
}

/* Panel_PlayerInit__FUsP6CBruce (0x8006e144) */
void Panel_PlayerInit(unsigned short n, CBruce *b) {
	if (b == MechList)
		b->mPanel.mPlayerNum = 0;
	else
		b->mPanel.mPlayerNum = 1;
}

/* Panel_TakeOff__FP6CBrucei (0x8006e168) */
/* C++. The skater takes off: sets the panel's trick status, clears old_180spins, starts the replay
   highlight. */
void Panel_TakeOff(CBruce *p, int a) {
	FUN_8006e7fc(&p->mPanel, a);

	p->mPanel.old_180spins = 0;
	Video_HighlightStart(p);
}

/* Panel_Trick__FP6CBrucePcii (0x8006e19c) */
void Panel_Trick(CBruce *p, char *name, int score, int a3) {
	p->mPanel.Trick(name, score, a3);
}

/* Panel_HoldTrick__FP6CBrucei (0x8006e1bc) */
void Panel_HoldTrick(CBruce *p, int s) {
	p->mPanel.mTQ[p->mPanel.mTQStack - 1].point_value += s;
	p->mPanel.TrickMessageRefresh();
}

/* Panel_Bonus__FP6CBrucei (0x8006e204) */
void Panel_Bonus(void) {}

/* Panel_Land__FP6CBruce (0x8006e20c) */
void Panel_Land(CBruce *p) {
	p->mPanel.Land();
}

/* Panel_Bail__FP6CBruce (0x8006e22c) */
void Panel_Bail(CBruce *p) {
	FUN_8006ea4c(&p->mPanel);
}

/* Panel_Letter__Fi (0x8006e24c) */
/* C++: the skater's S-K-A-T-E letters are CBruce::mPanel.SKATE (game/bruce.h, game/panel.h). */
int Panel_Letter(int n) {
	SKATE_Display = 40;

	if ((unsigned)n < 5)
		MechList->mPanel.SKATE[n] = 1;

	if (MechList->mPanel.SKATE[0] == 1 && MechList->mPanel.SKATE[1] == 1 &&
	    MechList->mPanel.SKATE[2] == 1 && MechList->mPanel.SKATE[3] == 1 &&
	    MechList->mPanel.SKATE[4] == 1) {
		Panel_Tape(2);
		return 1;
	}

	return 0;
}

/* Panel_Tape__Fi (0x8006e2fc) */
void Panel_Tape(int a) {
	TAPES_Display = 40;
	Career_GiveTape(GLevel, a);

	switch (a) {
		case 2:
			Mess_Message(GameText.pSKATETape, &Messprog_Goal, 1, 0, 0);
			break;
		case 0:
			SFX_PlayX(SND_COLLECT70, 0x2000, 0);
			Mess_Message(GameText.p20000Tape, &Messprog_Goal, 1, 0, 0);
			break;
		case 4:
			SFX_PlayX(SND_COLLECT70, 0x2000, 0);
			Mess_Message(GameText.p40000Tape, &Messprog_Goal, 1, 0, 0);
			break;
		case 1:
			SFX_PlayX(SND_COLLECT70, 0x2000, 0);
			sprintf(DAT_800d3868, GameText.p5GoalsTape, Levels[GLevel].pGoalName);
			Mess_Message(DAT_800d3868, &Messprog_Goal, 1, 0, 0);
			break;
		case 3:
			SFX_PlayX(SND_SECRETTAPE, 0x2000, 0);
			Mess_Message(GameText.pHiddenTape, &Messprog_Goal, 1, 0, 0);
			break;
	}
}

/* Panel_GoalCounter__Fi (0x8006e454) */
/* C++. Counts a skater event (+0xe00) in career mode: the 5th ends it (99, Panel_Tape__Fi(1));
   before that it shows "n of N" for the current level. */
void Panel_GoalCounter(void) {
	if (GGame != 1)
		return;

	CBruce *pSkater = MechList;
	int n = ++pSkater->mPanel.Goals;

	if (n == 5) {
		pSkater->mPanel.Goals = 99;
		Panel_Tape(1);
		return;
	}

	sprintf(Msg, "%d OF 5 %s", n, Levels[GLevel].pGoalName);
	Mess_Message(Msg, Messprog_Zoom, 1, 0, 0);
}

/* Set__6CTrickP7MessagePcii (0x8006e514) */
/* Fills in a trick queue entry. */
void CTrick::Set(Message *pMessage, char *pName, int Points, int Depreciate) {
	message = pMessage;
	name = pName;
	point_value = Points;
	countdown = 18;
	depreciate = Depreciate;
}

/* Panel_SwitchText__FP6CBruce (0x8006e534) */
/* C++: "SWITCH " when the skater rides against its stance (Stats[].pBio[4], "Regular"/"Goofy"). */
char *Panel_SwitchText(CBruce *p) {
	int r = 0;
	unsigned w = p->mExtraFlags;
	char c = *Stats[p->mSkaterIndex].pBio[4];
	unsigned b = w >> 1;

	b &= 1;

	if (c == 'R' || c == 'r')
		r = 1;

	if (b != r)
		return (char *)"SWITCH ";

	return (char *)"";
}

/* TrickMessageRefresh__10CPanelData (0x8006e5ac) */
/* Demo twin CPanelData::TrickMessageRefresh: THPS1 drops the per-trick spin
   display. Rebuilds the up-to-three trick-name lines: an optional spin prefix ("%d "), the
   stance name and the first trick, then each further trick appended to the current line,
   wrapping when the line gets wider than the viewport allows. sprintf = sprintf,
   Mess_TextWidth__FPCc = text width, Panel_SwitchText = "SWITCH " or "" prefix. */
void CPanelData::TrickMessageRefresh() {
	char *p;
	char tbuf[96];
	int lineWidth;
	int currLine;

	mTrickName2[0] = 0;
	mTrickName3[0] = 0;

	if (mpBruce->mNum180Spins > 0 && (mpBruce->mPhysicsState != 2 || mpBruce->mNum180Spins >= 2))
		p = mTrickName + sprintf(mTrickName, "%d ", mpBruce->mNum180Spins * 180);
	else
		p = mTrickName;

	currLine = 0;

	switch (ViewportMode * 2 + mpBruce->mPanel.mPlayerNum) {
		case 2:
		case 3:
		case 6:
		case 7:
			lineWidth = 240;
			break;
		default:
			lineWidth = 480;
			break;
	}

	if (!mTQStack) {
		mTrickName[0] = 0;
		return;
	}

	sprintf(p, "%s%s", Panel_SwitchText(mpBruce), mTQ[0].name);
	Mess_SetScale(188);

	for (int i = 1; i < mTQStack; i++) {
		sprintf(tbuf, "%s + %s", p, mTQ[i].name);

		if (Mess_TextWidth(tbuf) > lineWidth) {
			currLine++;

			if (currLine == 1)
				p = mTrickName2;
			else if (currLine == 2)
				p = mTrickName3;

			if (currLine == 3) {
				sprintf(p, "%s + ^...^", p);
				return;
			}

			sprintf(p, "+ %s", mTQ[i].name);
		} else
			sprintf(p, tbuf);
	}
}

/* TrickMessage__10CPanelData (0x8006e7c0) */
/* Starts showing the trick message (and refreshes its text) while tricks are
   queued; otherwise turns it off. */
void CPanelData::TrickMessage() {
	if (mTQStack) {
		mTrickState = 1;
		mTrickTimer = 0;
		TrickMessageRefresh();
	} else
		mTrickState = 0;
}

/* FUN_8006e7fc (0x8006e7fc) */
/* : CPanelData's trick-status setter. */
extern "C" void FUN_8006e7fc(CPanelData *p, int v) {
	p->mTrickStatus = v;
}

/* Trick__10CPanelDataPcii (0x8006e804) */
/* Demo twin CPanelData::Trick (656 B); THPS1 drops the spin/lock bookkeeping,
   shrinks CTrick to 20 bytes and adds the trick-status check. Queues a trick on the panel's trick
   queue, banking the previous landed total when the queue is empty. FUN_8006e514 = CTrick::Set,
   FUN_8006e7c0 = TrickMessage, the panel refresh. Called via the CBruce wrapper at 0x8006e19c (&mPanel = +0x924). */
/* points x10 cheat */
void CPanelData::Trick(char *pName, int Points, int Dep) {
	mLastHoldingScore = 1;

	if (DAT_800d1ec4)
		Points *= 10;

	switch (mTrickStatus) {
		case 0:
		case 1:
		case 2:
			if (Points > 0) {
				if (!mTQStack) {
					mTotalScore += mLandedTotal;
					mLandedTotal = 0;
					mLandedMultiplier = 0;
					m4F4 = 0;
				}

				mTQ[mTQStack].Set(0, pName, Points, Dep);

				if (mTQStack < 19)
					mTQStack++;
			}

			break;
		default:
			printf("Unexpected trick status: %d\n", mTrickStatus);
			break;
	}

	TrickMessage();
}

/* Land__10CPanelData (0x8006e8ec) */
/* Demo twin. Banks a landed combo: sums the trick scores, applies the multiplier
   (combo length plus a spin bonus), keeps the best combo's names, and ends the replay highlight. */
void CPanelData::Land() {
	mLastHoldingScore = 0;
	mLastMult = 0;

	if (mTQStack > 0) {
		int spins, mult;
		uint score;

		mLandedScore = 0;
		mNumTricksLanded += mTQStack;

		for (int i = 0; i < mTQStack; i++)
			mLandedScore += mTQ[i].point_value;

		spins = mpBruce->mNum180Spins;

		if (spins < 3)
			mult = mTQStack * 2 + spins;
		else
			mult = mTQStack * 2 + (spins * 2 - 2);

		mLandedMultiplier = mult;
		score = (mLandedScore * mult) >> 1;
		mLandedTotal += score;

		if (mBestComboScore < score) {
			mBestComboScore = score;
			mNumTricksInCombo = mTQStack;
			strcpy(mBestComboName, mTrickName);
			strcpy(mBestComboName2, mTrickName2);
			strcpy(mBestComboName3, mTrickName3);
		}

		mTQStack = 0;
		m4F4 = 40;

		if (GCareer.Options.ScoreDisplayEnabled == 0)
			m4F4 = 0;

		if (GGame == 2 && mLandedScore != 0)
			mTotalScore = 0;
	}

	Video_HighlightEnd(mpBruce, mLandedTotal);
}

/* FUN_8006ea4c (0x8006ea4c) */
/* C++: a CPanelData member taking the panel as its this (a bail): counts it, ends the replay
   highlight with the queued tricks' value (scaled by the skater's 180 spins) taken off, and starts
   the falling text if a holding score was pending; the trick queue empties. */
extern "C" void FUN_8006ea4c(CPanelData *p) {
	int sum = 0;
	int n;
	int i;
	int v;
	int t;

	p->mNumBails += 1;
	n = p->mTQStack;

	if (n > 0) {
		for (i = 0; i < n; i++)
			sum += p->mTQ[i].point_value;

		v = p->mpBruce->mNum180Spins;
		t = n * 2;
		t = n * 2;

		{
			int k = v;

			if (k < 3)
				v = t + k;
			else {
				int u = t - 2;

				v = u + k * 2;
			}
		}

		sum = sum * v / 2;
	}

	Video_HighlightEnd(p->mpBruce, -sum);

	if (p->mLastHoldingScore) {
		p->mLastHoldingScore = 0;
		p->mLastMult = 1;
		p->mFallingTextEffect.init();
	}

	p->mTQStack = 0;
}

/* FUN_8006eb18 (0x8006eb18) */
extern "C" void FUN_8006eb18(CPanelData *p) {
	short x, y;

	Panel_GetViewportMessOffset(ViewportMode * 2 + p->mpBruce->mPanel.mPlayerNum, &x, &y);

	if (p->mpBruce == GSkater[0])
		Mess_Message(GameText.pBlueSteals, &Messprog_Goal, 0, x, y);
	else
		Mess_Message(GameText.pRedSteals, &Messprog_Goal, 0, x, y);
}

/* OutOfBounds__10CPanelData (0x8006eba4) */
/* Picks one of the four out-of-bounds messages and restarts its falling text. */
void CPanelData::OutOfBounds() {
	mOutOfBoundsMessage = Rnd(4) + 1;
	mOutOfBoundsText.init();
}

/* TotalScore__10CPanelData (0x8006ebdc) */
/* Sums the point values of the tricks in the queue. */
int CPanelData::TotalScore() {
	int Total = 0;

	for (int i = 0; i < mTQStack; i++)
		Total += mTQ[i].point_value;

	return Total;
}
