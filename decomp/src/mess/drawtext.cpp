/* Mess_DrawText__FiiPCciUiib (0x8001e114) flags=-fstructure-noalias */
/* C++ (MESS.CPP). Fuzzy demo twin; locals, their order and types from the demo's MAIN.SYM, statements
   follow its line table. Draws a string right to left as one textured quad plus two shadow quads
   per character, handling the in-string control characters (scale toggle, colour changes, space).
   THPS1 adds an optional wobble/clip descriptor (DAT_800d15e8) that bounces characters vertically. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

typedef unsigned char uchar;

struct Character {
	int w, h, x, y, baseline;
	u_char u0, v0;
	u_short clut;
	u_char u1, v1;
	u_short tp;
	u_char u2, v2, u3, v3;
	int real_w;
};

struct FontInfo {
	char mFont_name[16];
	int mNum_chars;
	Character *mChar_table;
	u_char mFont_shadow_offset;
	u_short mFont_scale;
	int mChar_sep;
	int mSpace_width;
};

struct SWobble {
	short Offsets[32];
	u_short Limit;
	u_short Count;
	short pad44;
	u_short ClipY;
};

struct SDB {
	char pad0[0x84];
	u_long OT[3];
};

extern FontInfo Font_table[10];

int Current_font;
SWobble *DAT_800d15e8;
uchar TextJustification;
u_int FlagsBGR;
u_short Sort;
u_int LastFlagsBGR;

extern SDB *pDoubleBuffer;
extern uchar *pPoly;
extern uchar *PolyBufferEnd;

int Mess_TextWidth(const char *);
int Mess_GetScale(void);
void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);
int GetCharIndex(char);
void Panel_addPrim(void *);
extern "C" unsigned int strlen(const char *);

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
		DAT_800d15e8->Count = char_count;

	ScreenX += length;
	pScan += char_count;

	while (char_count) {
		int ypos = y;

		if (DAT_800d15e8) {
			int d = DAT_800d15e8->Offsets[char_count & 31];

			if (d < 0) {
				int e = -d;

				if (y + e * e >= DAT_800d15e8->Limit) {
					ypos = DAT_800d15e8->Limit;
					DAT_800d15e8->Count--;
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

			apoly->tag = 0x0c000000;
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
				if (apoly->y0 > DAT_800d15e8->ClipY || apoly->y1 > DAT_800d15e8->ClipY) {
					apoly->y2 = apoly->y0;
					apoly->y3 = apoly->y1;
				} else if (DAT_800d15e8->ClipY < apoly->y2 || DAT_800d15e8->ClipY < apoly->y3) {
					apoly->y2 = DAT_800d15e8->ClipY;
					apoly->y3 = DAT_800d15e8->ClipY;
				}
			}

			shadow->tag = 0x09000000;
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

			{
				u_int w0, w1, w2, w3;

				w0 = ((u_int *)shadow)[0];
				w1 = ((u_int *)shadow)[1];
				w2 = ((u_int *)shadow)[2];
				w3 = ((u_int *)shadow)[3];
				((u_int *)anothershadow)[0] = w0;
				((u_int *)anothershadow)[1] = w1;
				((u_int *)anothershadow)[2] = w2;
				((u_int *)anothershadow)[3] = w3;
				w2 += shadow_off;
				((u_int *)shadow)[2] = w2;
				w0 = ((u_int *)shadow)[4];
				w1 = ((u_int *)shadow)[5];
				w2 = ((u_int *)shadow)[6];
				w3 = ((u_int *)shadow)[7];
				((u_int *)anothershadow)[4] = w0;
				((u_int *)anothershadow)[5] = w1;
				((u_int *)anothershadow)[6] = w2;
				((u_int *)anothershadow)[7] = w3;
				w0 += shadow_off;
				((u_int *)shadow)[4] = w0;
				w2 += shadow_off;
				((u_int *)shadow)[6] = w2;
				w0 = ((u_int *)shadow)[8];
				w1 = ((u_int *)shadow)[9];
				((u_int *)anothershadow)[8] = w0;
				((u_int *)anothershadow)[9] = w1;
				w0 += shadow_off;
				((u_int *)shadow)[8] = w0;
			}

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

			addPrim(&pDoubleBuffer->OT[Sort], apoly);
			addPrim(&pDoubleBuffer->OT[Sort], anothershadow);
			addPrim(&pDoubleBuffer->OT[Sort], shadow);
		}
	}

	return length;
}
