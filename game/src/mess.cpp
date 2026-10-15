/* MESS.CPP: the panel text printer: the old fixed font and the loaded .FNT fonts (Mess_DrawText,
   Mess_LoadFont), the scripted on-screen messages (Mess_Message) and the falling-letters
   FallingTextEffect (declarations in game/mess.h; font types in game/font.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/mess.h"
#include "game/m3d.h"
#include "game/font.h"
#include "game/psyq.h"
#include "game/mem.h"
#include "game/fileio.h"
#include "game/pack.h"
#include "game/utils.h"
#include "game/bit.h"
#include "game/globals.h"
#include "game/pal.h"
#include "game/panel.h"
#include "game/shellutils.h"
#include "pc_texstore.h"



extern Message *pMessages;
extern uchar TextJustification;
extern uchar NewFont;
extern ushort Scale;
extern ushort Sort;
extern uint FlagsBGR;
extern uint LastFlagsBGR;
extern FontInfo Font_table[10];
extern int Num_fonts, Current_font;
extern u_short Clut;
/* the old fixed font */
extern uchar BigFont_Mapping[];
/* the falling-text effect Mess_DrawText applies (FallingTextEffect::bind) */
extern FallingTextEffect *DAT_800d15e8;
/* Mess_DrawText's view of the bound FallingTextEffect (DAT_800d15e8): it reads the floor and
   clip lines (game/mess.h m40/m46, declared signed there) as unsigned */
struct SFallingTextView {
	short mWaitTime[32]; /* 0x00 */
	u_short mFloorY;     /* 0x40: m40, the line the characters fall to */
	u_short m42;         /* 0x42 */
	ushort mScale;       /* 0x44 */
	u_short mClipY;      /* 0x46: m46, characters are cut off below it */
};
/* FUN_8001f274's object: vptr, an owned buffer and its owned flag */
struct SOwnedBuffer {
	char *vt;
	void *pData;
	int Owned;
};
/* vtables of FUN_8001f274's / FUN_8001f2dc's class and its base */
extern char DAT_8009ef94[];
extern char DAT_8009efa4[];

int Mess_TextHeight2(const char *);
int GetCharIndex(char);
void Mess_SetNewFont(uchar);
Message *CreateMessage(void);
void Mess_DeleteMessage(Message *);
extern "C" int FUN_8001d340(const uchar *);
extern "C" int FUN_8001d548(const uchar *);
extern "C" void FUN_8008f6c8(void *);
extern "C" void __dl__6CClassPv(void *p);

/* Mess_SetTextJustify__FUc (0x8001d250) */
void Mess_SetTextJustify(uchar v) {
	TextJustification = v;
}

/* Mess_SetScale__Fi (0x8001d25c) */
void Mess_SetScale(int s) {
	if (NewFont)
		Mess_SetFontScale(s);
	else
		Scale = s;
}

/* Mess_GetScale__Fv (0x8001d290) */
int Mess_GetScale(void) {
	if (NewFont)
		return Mess_GetFontScale();

	return Scale;
}

/* Mess_SetSort__Fi (0x8001d2c4) */
void Mess_SetSort(int v) {
	Sort = v;
}

/* Mess_SetRGB__FUcUcUci (0x8001d2d0) */
void Mess_SetRGB(uchar r, uchar g, uchar b, int flag) {
	unsigned rr = r & 0xff, gg = g & 0xff, bb = b & 0xff;
	unsigned c = (bb << 16) | (gg << 8) | rr;

	FlagsBGR = c;
	FlagsBGR = c | (flag ? 0x2e000000u : 0x2c000000u);
}

/* Mess_DrawPanelText__FiiPCciUii (0x8001d30c) */
void Mess_DrawPanelText(int x, int y, const char *s, int a, unsigned b, int c) {
	Mess_DrawText(x, y, s, a, b, c, 1);
}

/* FUN_8001d340 (0x8001d340) */
/* an SAnimFrame (game/m3d.h; BigFontAnim is Spool_FindAnim's): its texture pointer makes the
   stride 16 bytes on x64 */
extern "C" int FUN_8001d340(const uchar *s) {
	int w = 0;
	int n = 0;

	while (*s) {
		int v = BigFont_Mapping[*s];

		if (v == 254)
			v = -1;

		if (v == 253)
			v = -1;

		if (v == 252)
			v = -1;

		if (v == 251)
			v = -1;

		if (v == 250)
			v = -1;

		if (v == 255)
			v = -1;

		if (v >= 0) {
			n++;
			w += BigFontAnim[v].Width + BigFontAnim[v].OffX;
		} else
			w += 10;

		s++;
	}

	w = (w * Scale) >> 8;
	return w + n * 3;
}

/* Mess_TextHeight2__FPCc (0x8001d440) */
int Mess_TextHeight2(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m = 0;

	while (*s) {
		int v = BigFont_Mapping[*s];

		if (v == 254)
			v = -1;

		if (v == 253)
			v = -1;

		if (v == 252)
			v = -1;

		if (v == 251)
			v = -1;

		if (v == 250)
			v = -1;

		if (v == 255)
			v = -1;

		if (v >= 0) {
			int w = BigFontAnim[v].Height;

			if (m < w)
				m = w;
		}

		s++;
	}

	return (m * Scale) >> 8;
}

/* Mess_TextHeightAboveBaseline2__FPCc (0x8001d528) */
/* The old-font version of Mess_TextHeightAboveBaseline (which calls it when
   !NewFont): a tail call to Mess_TextHeight2, as in the demo (MESS.CPP order Mess_TextHeight2,
   Mess_TextHeightAboveBaseline2, Mess_TextHeightBelowBaseline2 = retail 0x8001d440/d528/d548).
   Was mislabelled CBruce::Trick_HoldTrick, which is 0x8002f438. */
int Mess_TextHeightAboveBaseline2(const char *s) {
	return Mess_TextHeight2(s);
}

/* FUN_8001d548 (0x8001d548) */
/* the old font's Mess_TextHeightBelowBaseline: none */
extern "C" int FUN_8001d548(const uchar *s) {
	return 0;
}

/* CreateMessage__Fv (0x8001d550) */
/* the whole layout (Mess_Message, Mess_Update): in the 64-bit port text/prog are 8 bytes */
/* the new message, first in the list (Mess_Message uses it) */
Message *CreateMessage(void) {
	Message *m = (Message *)Mem_NewX(sizeof(Message), 0, 1);
	Message *h = pMessages;

	m->pPrev = 0;
	pMessages = m;
	m->pNext = h;

	if (h)
		h->pPrev = m;

	return m;
}

/* Mess_DeleteMessage__FP7Message (0x8001d590) */
/* the whole layout (Mess_Message, Mess_Update): in the 64-bit port text/prog are 8 bytes */
void Mess_DeleteMessage(Message *m) {
	if (m->pNext)
		m->pNext->pPrev = m->pPrev;

	if (m->pPrev)
		m->pPrev->pNext = m->pNext;

	if (m == pMessages)
		pMessages = m->pNext;

	Mem_Delete(m);
}

/* Mess_DeleteAll__Fv (0x8001d5fc) */
/* the whole layout (Mess_Message, Mess_Update): in the 64-bit port text/prog are 8 bytes */
void Mess_DeleteAll(void) {
	Message *m = pMessages;
	Message *n;

	if (m) {
		n = m->pNext;
	loop:
		Mem_Delete(m);

		if (n) {
			m = n;
			n = n->pNext;
			goto loop;
		}
	}

	pMessages = 0;
}

/* Mess_Message__FPCcP12SMessageProgbss (0x8001d644) */
Message *Mess_Message(const char *text, SMessageProg *prog, bool flag, short x, short y) {
	Message *m = CreateMessage();

	m->pText = text;
	m->pProg = prog;
	m->Timer = 0;
	m->X = x;
	m->Y = y;
	m->Stacked = flag;
	m->TotalTime = 0;
	return m;
}

/* Mess_Update__Fv (0x8001d6bc) */
void Mess_Update(void) {
	Message *m = pMessages;
	Message *n;

	if (!m)
		return;

	n = m->pNext;

	for (;;) {
		SMessageProg *p = m->pProg;

		if (m->Timer >= p->Duration) {
			signed char c = p->Next;

			if (c == 0) {
				Mess_DeleteMessage(m);
			} else if (c != 127) {
				m->pProg = p + c;
				m->Timer = 0;
				m->TotalTime++;
			}
		} else {
			m->Timer++;
			m->TotalTime++;
		}

		m = n;

		if (n)
			n = n->pNext;
		else
			break;
	}
}

/* Mess_Display__Fv (0x8001d764) */
void Mess_Display(void) {
	Message *head;
	Message *n;
	SMessageProg *a;
	int cnt, off, x, y, w, v, d;
	unsigned char cr, cg, cb;
	int sc, q4;
	unsigned short Dur;

	head = n = pMessages;
	cnt = 0;

	for (; n; n = n->pNext)
		if (n->Stacked)
			cnt++;

	off = (cnt - 1) << 4;

	for (n = head; n; n = n->pNext) {
		a = n->pProg;

		if (a->BlinkRate && !((n->Timer / a->BlinkRate) & 1))
			continue;

		Dur = a->Duration;
		TextJustification = a->Justify;
		Scale = ((a->Scale1 - a->Scale0) * n->Timer) / Dur + a->Scale0;
		Mess_SetScale(Scale);

		cr = a->R0 + ((a->R1 - a->R0) * n->Timer) / Dur;
		cg = a->G0 + ((a->G1 - a->G0) * n->Timer) / Dur;
		cb = a->B0 + ((a->B1 - a->B0) * n->Timer) / Dur;
		v = (cb << 16) | (cg << 8) | cr;
		FlagsBGR = v;

		if (a->SemiTrans)
			FlagsBGR |= 0x2e000000;
		else
			FlagsBGR |= 0x2c000000;

		if (a->ColourCycle)
			Mess_SetRGB(((Xblanks % 20) * 12) & 252, 100, 100, 0);

		y = a->Y0 + ((a->Y1 - a->Y0) * n->Timer) / Dur + n->Y;
		x = a->X0 + ((a->X1 - a->X0) * n->Timer) / Dur + n->X;

		if (n->Stacked) {
			y += off;
			off -= 32;
		}

		w = a->Solid0 + ((a->Solid1 - a->Solid0) * n->Timer) / Dur;
		Mess_DrawPanelText(x, y, n->pText, w, 0, 256);
	}
}

/* Mess_TextWidth__FPCc (0x8001da44) */
/* C++. A string's width in the current font, scaled (the old fixed font: FUN_8001d340): each
   character's width (real_w; a space: the font's space width) plus the letter spacing. */
int Mess_TextWidth(const char *s0) {
	int n, idx;
	const uchar *s;

	if (NewFont == 0)
		return FUN_8001d340((const uchar *)s0);

	s = (const uchar *)s0;
	n = 0;

	if (*s) {
		do {
			idx = GetCharIndex(*s);

			if ((unsigned)(idx - 250) >= 5) {
				if (idx == 255) {
					n += ((Font_table[Current_font].mSpace_width + Font_table[Current_font].mChar_sep) *
					      Font_table[Current_font].mFont_scale) >>
					     8;
				} else {
					n += ((Font_table[Current_font].mChar_table[idx].real_w +
					       Font_table[Current_font].mChar_sep) *
					      Font_table[Current_font].mFont_scale) >>
					     8;
				}
			}

			s++;
		} while (*s);
	}

	return n;
}

/* Mess_TextHeightBelowBaseline__FPCc (0x8001db80) */
int Mess_TextHeightBelowBaseline(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m;
	int v;
	int c;
	const uchar *p;

	if (!NewFont)
		return FUN_8001d548(s);

	m = 0;
	p = s;

	while (*p) {
		c = GetCharIndex(*p);

		if ((unsigned)(c - 250) < 5)
			v = 0;
		else if (c == 255)
			v = 0;
		else
			v = Font_table[Current_font].mChar_table[c].h - Font_table[Current_font].mChar_table[c].baseline;

		if (m < v)
			m = v;

		p++;
	}

	return (m * Font_table[Current_font].mFont_scale) >> 8;
}

/* Mess_TextHeightAboveBaseline__FPCc (0x8001dc9c) */
int Mess_TextHeightAboveBaseline(const char *s0) {
	const uchar *s = (const uchar *)s0;
	int m;
	int v;
	int c;
	const uchar *p;

	if (!NewFont)
		return Mess_TextHeightAboveBaseline2(s0);

	m = 0;
	p = s;

	while (*p) {
		c = GetCharIndex(*p);

		if ((unsigned)(c - 250) < 5)
			v = 0;
		else if (c == 255)
			v = 0;
		else
			v = Font_table[Current_font].mChar_table[c].baseline;

		if (m < v)
			m = v;

		p++;
	}

	return (m * Font_table[Current_font].mFont_scale) >> 8;
}

/* Mess_TextHeight__FPCc (0x8001ddb0) */
int Mess_TextHeight(const char *s) {
	if (NewFont)
		return Mess_TextHeightAboveBaseline(s) + Mess_TextHeightBelowBaseline(s);

	return Mess_TextHeight2(s);
}

/* GetCharIndex__Fc (0x8001ddfc) */
/* MESS.CPP. Demo twin (THPS1 drops the bracket remaps). Maps a character to its font glyph index:
   digits and ':' only for the small numeric fonts, else letters, digits and punctuation. */
int GetCharIndex(char ch) {
	unsigned c;
	int v, r;

	c = ch & 255;

	if (Font_table[Current_font].mNum_chars < 15) {
		v = c - 48;

		if ((unsigned char)v < 10)
			return v;

		if (c == 58)
			return 10;

		return 0;
	}

	if (c == 32)
		return 255;

	if (c == 91)
		return 254;

	if (c == 93)
		return 253;

	if (c == 123)
		return 254;

	if (c == 125)
		return 252;

	if (c == 60)
		return 254;

	if (c == 62)
		return 251;

	if (c == 94)
		return 250;

	v = c - 65;

	if ((unsigned char)v < 26)
		return v;

	v = c - 97;

	if ((unsigned char)v < 26)
		return v;

	if (c - 48 < 10)
		return c - 22;

	switch (c) {
		case 33:
			return 37;
		case 58:
			return 38;
		case 46:
			return 39;
		case 44:
			return 40;
		case 45:
		case 95:
			return 41;
		case 37:
			return 42;
		case 43:
			return 43;
		case 39: {
			if (Font_table[Current_font].mNum_chars >= 45)
				return 44;
		}
			return 36;
		case 63:
			return 36;
		case 35:
			return 45;
		case 34:
			return 46;
		case 38:
			return 47;
		case 64:
			return 48;
		default:
			r = 36;
			break;
	}

	return r;
}

/* __17FallingTextEffect (0x8001dfc4) */
/* A falling-text effect starts initialised. */
FallingTextEffect::FallingTextEffect() {
	init();
}

/* FUN_8001dfec (0x8001dfec) */
extern "C" void FUN_8001dfec(FallingTextEffect *a, int b) {
	if (b & 1)
		FUN_8008f6c8(a);
}

/* init__17FallingTextEffect (0x8001e014) */
/* Gives each character a random fall delay of 15..44 frames and resets the
   effect's state words at 0x40/0x46 to 280 and 240. */
void FallingTextEffect::init() {
	for (int i = 0; i < 32; i++)
		mWaitTime[i] = Rnd(30) + 15;

	m40 = 280;
	m46 = 240;
}

/* update__17FallingTextEffect (0x8001e07c) */
/* Counts every character's fall delay down by one frame. */
void FallingTextEffect::update() {
	for (int i = 0; i < 32; i++)
		mWaitTime[i]--;
}

/* bind__17FallingTextEffecti (0x8001e0a4) */
/* Binds the effect to Mess_DrawText (On != 0: its scale, red text, and the global
   wobble descriptor DAT_800d15e8 points at it) or unbinds it. */
void FallingTextEffect::bind(int On) {
	if (On) {
		Mess_SetScale(mScale);
		Mess_SetRGB(255, 48, 48, 0);

		DAT_800d15e8 = this;
	} else
		DAT_800d15e8 = 0;
}

/* setScale__17FallingTextEffectUs (0x8001e0f8) */
/* Sets the scale bind() passes to Mess_SetScale. */
void FallingTextEffect::setScale(ushort Scale) {
	mScale = Scale;
}

/* FUN_8001e100 (0x8001e100) */
extern "C" void FUN_8001e100(FallingTextEffect *p, short v) {
	p->m46 = v;
}

/* handleComplete__17FallingTextEffect (0x8001e108) */
/* Returns m42. */
ushort FallingTextEffect::handleComplete() {
	return m42;
}

/* Mess_DrawText__FiiPCciUiib (0x8001e114) */
/* Fuzzy demo twin; locals, their order and types from the demo's MAIN.SYM, statements
   follow its line table. Draws a string right to left as one textured quad plus two shadow quads
   per character, handling the in-string control characters (scale toggle, colour changes, space).
   THPS1 adds an optional wobble/clip descriptor (DAT_800d15e8) that bounces characters vertically. */
int Mess_DrawText(int x, int y, const char *pMessage, int DrawSolid, u_int FireRGB, int FireSquash,
                  bool IsPanel) {
	const char *pScan;
	int Off;
	int length;
	int currentScale;
	int useCurrentScale;
	int ScreenX;
	u_int shadow_off;
	int n;
	int char_count;

	length = Mess_TextWidth(pMessage);
	currentScale = Mess_GetScale();
	useCurrentScale = 1;

	switch (TextJustification) {
		case 0:
			Off = -length / 2;
			break;
		case 2:
			Off = -length;
			break;
		case 1:
			Off = 0;
			break;
	}

	ScreenX = x + Off;
	shadow_off = (((u_int)(Font_table[Current_font].mFont_shadow_offset *
	                       Font_table[Current_font].mFont_scale) >>
	               8)
	              << 16) +
	             ((u_int)(Font_table[Current_font].mFont_shadow_offset *
	                      Font_table[Current_font].mFont_scale) >>
	              8);
	pScan = pMessage;
	char_count = strlen(pScan);

	if (DAT_800d15e8)
		DAT_800d15e8->m42 = char_count;

	ScreenX += length;
	pScan += char_count;

	while (char_count) {
		int ypos = y;

		if (DAT_800d15e8) {
			int d = DAT_800d15e8->mWaitTime[char_count & 31];

			if (d < 0) {
				int e = -d;

				if (y + e * e >= ((SFallingTextView *)DAT_800d15e8)->mFloorY) {
					ypos = ((SFallingTextView *)DAT_800d15e8)->mFloorY;
					DAT_800d15e8->m42--;
				} else
					ypos = y + e * e;
			}
		}

		pScan--;
		n = GetCharIndex(*pScan);
		char_count--;

		if (n == 250) {
			useCurrentScale = !useCurrentScale;
			Mess_SetScale(useCurrentScale ? currentScale : 256);
			continue;
		}

		if (n == 254) {
			if (!DAT_800d15e8)
				FlagsBGR = LastFlagsBGR;

			continue;
		}

		if (n == 253) {
			if (!DAT_800d15e8) {
				LastFlagsBGR = FlagsBGR;
				Mess_SetRGB(0, 128, 255, 0);
			}

			continue;
		}

		if (n == 252) {
			if (!DAT_800d15e8) {
				LastFlagsBGR = FlagsBGR;
				Mess_SetRGB(255, 128, 0, 0);
			}

			continue;
		}

		if (n == 251) {
			if (!DAT_800d15e8) {
				LastFlagsBGR = FlagsBGR;
				Mess_SetRGB(64, 64, 64, 0);
			}

			continue;
		}

		if (n == 255) {
			ScreenX -=
			    ((Font_table[Current_font].mSpace_width + Font_table[Current_font].mChar_sep) *
				 Font_table[Current_font].mFont_scale) >>
			    8;
			continue;
		}

		ScreenX -=
		    ((Font_table[Current_font].mChar_table[n].real_w + Font_table[Current_font].mChar_sep) *
			 Font_table[Current_font].mFont_scale) >>
		    8;

		{
			POLY_GT4 *apoly;
			POLY_FT4 *shadow;
			POLY_FT4 *anothershadow;
			uchar *newppoly;
			u_int u0;
			u_int u1;
			u_short u2;
			u_short u3;
			uchar LargestRGB;

			apoly = (POLY_GT4 *)pPoly;
			shadow = (POLY_FT4 *)(apoly + 1);
			anothershadow = shadow + 1;
			newppoly = (uchar *)(anothershadow + 1);

			if (newppoly > PolyBufferEnd)
				break;

			pPoly = newppoly;

			setlen(apoly, 12);
			*(u_long *)&apoly->r0 = (FlagsBGR & 0xffffff) + 0x3e000000;
			*(u_long *)&apoly->r1 = FlagsBGR;
			*(u_long *)&apoly->r2 = FlagsBGR;
			*(u_long *)&apoly->r3 = FlagsBGR;

			u0 = *(u_int *)&Font_table[Current_font].mChar_table[n].u0;
			u1 = *(u_int *)&Font_table[Current_font].mChar_table[n].u1;
			u2 = *(u_short *)&Font_table[Current_font].mChar_table[n].u2;
			u3 = *(u_short *)&Font_table[Current_font].mChar_table[n].u3;

			if (Font_table[Current_font].mFont_scale > 256) {
				u1--;
				u2 -= 256;
				u3 -= 257;
			}

			*(u_int *)&apoly->u0 = u0;
			*(u_int *)&apoly->u1 = u1;
			*(u_short *)&apoly->u2 = u2;
			*(u_short *)&apoly->u3 = u3;

			apoly->x0 = ScreenX;
			apoly->y0 = ypos - ((Font_table[Current_font].mChar_table[n].baseline *
			                     Font_table[Current_font].mFont_scale) >>
			                    8);
			apoly->x1 = ScreenX + (((Font_table[Current_font].mChar_table[n].w << 2) *
			                        Font_table[Current_font].mFont_scale) >>
			                       8);
			apoly->y1 = apoly->y0;
			apoly->x2 = apoly->x0;
			apoly->y2 = apoly->y0 + ((Font_table[Current_font].mChar_table[n].h *
			                          Font_table[Current_font].mFont_scale) >>
			                         8);
			apoly->x3 = apoly->x1;
			apoly->y3 = apoly->y2;

			setSemiTrans(apoly, 0);

			if (!DrawSolid) {
				setSemiTrans(apoly, 1);

				apoly->tpage = (apoly->tpage & ~0x60) | 0x20;
			}

			if (DAT_800d15e8) {
				if (apoly->y0 > ((SFallingTextView *)DAT_800d15e8)->mClipY ||
				    apoly->y1 > ((SFallingTextView *)DAT_800d15e8)->mClipY) {
					apoly->y2 = apoly->y0;
					apoly->y3 = apoly->y1;
				} else if (((SFallingTextView *)DAT_800d15e8)->mClipY < apoly->y2 ||
				           ((SFallingTextView *)DAT_800d15e8)->mClipY < apoly->y3) {
					apoly->y2 = ((SFallingTextView *)DAT_800d15e8)->mClipY;
					apoly->y3 = ((SFallingTextView *)DAT_800d15e8)->mClipY;
				}
			}

			setlen(shadow, 9);
			*(u_long *)&shadow->r0 = FlagsBGR;
			*(u_int *)&shadow->u0 = u0;
			*(u_int *)&shadow->u1 = u1;
			*(u_short *)&shadow->u2 = u2;
			*(u_short *)&shadow->u3 = u3;
			shadow->x0 = apoly->x0;
			shadow->y0 = apoly->y0;
			shadow->x1 = apoly->x1;
			shadow->y1 = apoly->y1;
			shadow->x2 = apoly->x2;
			shadow->y2 = apoly->y2;
			shadow->x3 = apoly->x3;
			shadow->y3 = apoly->y3;
			setSemiTrans(shadow, 1);

			shadow->tpage = (shadow->tpage & ~0x60) | 0x40;
			shadow->r0 = 128;
			shadow->g0 = 128;
			shadow->b0 = 128;

			/* a second shadow where this one was; this one moves by shadow_off (added to each
			   screen word, x low and y high) */
			*anothershadow = *shadow;
			*(u_int *)&shadow->x0 += shadow_off;
			*(u_int *)&shadow->x1 += shadow_off;
			*(u_int *)&shadow->x2 += shadow_off;
			*(u_int *)&shadow->x3 += shadow_off;

			LargestRGB = 128;
			anothershadow->r0 = LargestRGB;
			anothershadow->g0 = LargestRGB;
			anothershadow->b0 = LargestRGB;
			setSemiTrans(anothershadow, 1);

			anothershadow->tpage = (anothershadow->tpage & ~0x60) | 0x40;

			if (IsPanel) {
				Panel_addPrim(apoly);
				Panel_addPrim(anothershadow);
				Panel_addPrim(shadow);
				continue;
			}

			addPrim(&(&pDoubleBuffer->OT2D)[Sort], apoly);
			addPrim(&(&pDoubleBuffer->OT2D)[Sort], anothershadow);
			addPrim(&(&pDoubleBuffer->OT2D)[Sort], shadow);
		}
	}

	return length;
}

/* Mess_LoadFont__FPcT0iii (0x8001e8c8) */
/* Locals, their order and types come from the demo's MAIN.SYM; statements follow its line table. */
int Mess_LoadFont(char *filename, char *file_buffer, int char_sep, int shadow_offset, int width) {
	int this_font;
	int clutx, cluty;
	u_short *clut;
	RECT r;
	char *db;
	int x, y;

	Mess_SetNewFont(1);

	this_font = Num_fonts;

	{
		int the_font;

		for (the_font = 0; the_font < Num_fonts; the_font++) {
			if (Font_table[the_font].mChar_table) {
				if (!strcmp(Font_table[the_font].mFont_name, filename)) {
					Mem_Delete(file_buffer);

					Current_font = the_font;
					return 0;
				}
			} else
				this_font = the_font;
		}
	}

	Pack_NoBottom = 1;
	Pack_NoRight = 1;

	if (this_font == Num_fonts)
		Num_fonts = this_font + 1;

	Font_table[this_font].mNum_chars = *(int *)file_buffer;
	strcpy(Font_table[this_font].mFont_name, filename);

	clutx = Pal16X;
	cluty = Pal16Y + GetFree16Slot();
	clut = (u_short *)Mem_New(32, 1, 1);

	{
		int i;

		for (i = 0; i < 16; i++)
			clut[i] = ((u_short *)(file_buffer + Font_table[this_font].mNum_chars * 16 + 4))[i];
	}

	r.x = clutx;
	r.y = cluty;
	r.w = 16;
	r.h = 1;
	PC_TexUpload(r.x, r.y, r.w, r.h, clut);
	Mem_Delete(clut);

	Clut = GetClut(clutx, cluty);

	Font_table[this_font].mChar_table =
	    (Character *)Mem_New(Font_table[this_font].mNum_chars * sizeof(Character), 0, 1);
	Font_table[this_font].mFont_scale = 256;

	if (!Font_table[this_font].mChar_table)
		printf("\nmemory error");

	db = file_buffer + (Font_table[this_font].mNum_chars * 16 + 4) + 32;
	char *tb = file_buffer + 4;

	{
		int i;

		for (i = 0; i < Font_table[this_font].mNum_chars; i++) {
			int w, h;
			RECT r;

			w = *(int *)tb;
			h = *(int *)(tb + 4);
			Pack_Pack(0, w * 4, h, &x, &y, 4, 0, 0);

			r.x = x;
			r.y = y;
			r.w = w;
			r.h = h;
			PC_TexUpload(r.x, r.y, r.w, r.h, db);
			DrawSync(0);

			Font_table[this_font].mChar_table[i].x = x;
			Font_table[this_font].mChar_table[i].y = y;
			Font_table[this_font].mChar_table[i].w = w;
			Font_table[this_font].mChar_table[i].h = h;
			Font_table[this_font].mChar_table[i].baseline = *(int *)(tb + 8);

			if (width == -1 || (width == 0 && i == 0))
				Font_table[this_font].mChar_table[i].real_w = *(int *)(tb + 12);
			else if (width == 0)
				Font_table[this_font].mChar_table[i].real_w =
				    Font_table[this_font].mChar_table[0].real_w;
			else
				Font_table[this_font].mChar_table[i].real_w = width;

			Font_table[this_font].mChar_table[i].u0 = x << 2;
			Font_table[this_font].mChar_table[i].v0 = y;
			Font_table[this_font].mChar_table[i].u1 =
			    Font_table[this_font].mChar_table[i].u0 + w * 4;
			Font_table[this_font].mChar_table[i].v1 = Font_table[this_font].mChar_table[i].v0;
			Font_table[this_font].mChar_table[i].u2 = Font_table[this_font].mChar_table[i].u0;
			Font_table[this_font].mChar_table[i].v2 = Font_table[this_font].mChar_table[i].v0 + h;
			Font_table[this_font].mChar_table[i].u3 = Font_table[this_font].mChar_table[i].u1;
			Font_table[this_font].mChar_table[i].v3 = Font_table[this_font].mChar_table[i].v2;
			Font_table[this_font].mChar_table[i].tp = GetTPage(0, 0, x, y);
			Font_table[this_font].mChar_table[i].clut = Clut;
			int pad = ((w * h) & 1) * 2;

			db += w * h * 2 + pad;
			tb += 16;
		}
	}

	Mem_Delete(file_buffer);

	if (shadow_offset == -1)
		Font_table[this_font].mFont_shadow_offset = Font_table[this_font].mChar_table[0].w >> 1;
	else
		Font_table[this_font].mFont_shadow_offset = shadow_offset;

	if (char_sep == -1)
		Font_table[this_font].mChar_sep = Font_table[this_font].mChar_table[0].w >> 2;
	else
		Font_table[this_font].mChar_sep = char_sep;

	Font_table[this_font].mSpace_width = Font_table[this_font].mChar_table[0].w * 28 / 10;

	Pack_NoBottom = 0;
	Pack_NoRight = 0;

	Current_font = this_font;
	return this_font;
}

/* Mess_LoadFont__FPciii (0x8001eed4) */
/* the font (Mess_SetCurrentFont) */
int Mess_LoadFont(char *name, int a, int b, int c) {
	void *buf = Mem_NewX(FileIO_Open(name), 1, 1);

	FileIO_Load(buf);
	FileIO_Sync();
	return Mess_LoadFont(name, (char *)buf, a, b, c);
}

/* Mess_UnloadFont__Fv (0x8001ef60) */
void Mess_UnloadFont(void) {
	int i;

	for (i = 0; i < Font_table[Current_font].mNum_chars; i++) {
		Character *p = &Font_table[Current_font].mChar_table[i];

		Pack_Unpack((short)p->x, (short)p->y);
	}

	Free16Slot(Font_table[Current_font].mChar_table[0].clut);
	Mem_Delete(Font_table[Current_font].mChar_table);

	Font_table[Current_font].mChar_table = 0;

	{
		int n = Num_fonts, i, m;

		if (Current_font == n - 1) {
			for (;;) {
				if (n <= 0)
					return;

				m = n - 1;
				i = n;
				Num_fonts = m;
				n = m;

				/* (with no fonts left retail looked at the word before the table; either way it
				   returns next time round) */
				if (i < 2 || Font_table[i - 2].mChar_table)
					return;
			}
		}
	}
}

/* Mess_UnloadAllFonts__Fv (0x8001f0dc) */
void Mess_UnloadAllFonts() {
	int i;

	for (i = 0; i < 10; i++) {
		if (Font_table[i].mChar_table) {
			Current_font = i;
			Mess_UnloadFont();
		}
	}
}

/* Mess_SetCurrentFont__Fi (0x8001f138) */
void Mess_SetCurrentFont(int v) {
	Current_font = v;
}

/* Mess_SetCurrentFont__FPc (0x8001f144) */
void Mess_SetCurrentFont(char *name) {
	int i;

	for (i = 0; i < Num_fonts; i++) {
		if (!strcmp(name, Font_table[i].mFont_name))
			break;
	}

	Current_font = i;
}

/* Mess_SetFontShadowOffset__FUc (0x8001f1b4) */
void Mess_SetFontShadowOffset(uchar v) {
	Font_table[Current_font].mFont_shadow_offset = v;
}

/* Mess_SetFontScale__FUi (0x8001f1d8) */
void Mess_SetFontScale(uint s) {
	if (!NewFont)
		Mess_SetScale(s);
	else
		Font_table[Current_font].mFont_scale = s;
}

/* Mess_GetFontScale__Fv (0x8001f22c) */
int Mess_GetFontScale() {
	if (!NewFont)
		return 0;

	return Font_table[Current_font].mFont_scale;
}

/* Mess_SetNewFont__FUc (0x8001f268) */
void Mess_SetNewFont(uchar v) {
	NewFont = v;
}

/* FUN_8001f274 (0x8001f274) */
/* the destructor of an object owning a buffer (SOwnedBuffer) */
extern "C" void FUN_8001f274(SOwnedBuffer *p, int f) {
	p->vt = DAT_8009ef94;

	if (p->Owned)
		Mem_Delete(p->pData);

	p->vt = DAT_8009efa4;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_8001f2dc (0x8001f2dc) */
extern "C" void FUN_8001f2dc(char *p, int f) {
	*(char **)(p + 0) = DAT_8009efa4;

	if (f & 1)
		__dl__6CClassPv(p);
}
