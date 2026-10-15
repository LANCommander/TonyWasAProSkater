/* Fonts: the proportional .FNT font the MESS text printer loads (Character, FontInfo; used by
   Mess_LoadFont / Mess_DrawText) and FONT.CPP's queued debug strings (SString). Layouts = demo.

   Not here: the front end's GameText table (SText in front/controller*) is LocalText in
   game/front.h. See build/hdr_notes/font.md. */
#ifndef GAME_FONT_H
#define GAME_FONT_H

#include "game/types.h"
#include "game/vector.h"

/* one glyph of a font */
struct Character {
	int w;        /* 0x00 */
	int h;        /* 0x04 */
	int x;        /* 0x08 */
	int y;        /* 0x0c */
	int baseline; /* 0x10 */
	uchar u0;     /* 0x14 */
	uchar v0;     /* 0x15 */
	ushort clut;  /* 0x16 */
	uchar u1;     /* 0x18 */
	uchar v1;     /* 0x19 */
	ushort tp;    /* 0x1a */
	uchar u2;     /* 0x1c */
	uchar v2;     /* 0x1d */
	uchar u3;     /* 0x1e */
	uchar v3;     /* 0x1f */
	int real_w;   /* 0x20 */
	              /* sizeof(Character) == 0x24 */
};

struct FontInfo {
	char mFont_name[16];       /* 0x00 */
	int mNum_chars;            /* 0x10 */
	Character *mChar_table;    /* 0x14 */
	uchar mFont_shadow_offset; /* 0x18 */
	ushort mFont_scale;        /* 0x1a */
	int mChar_sep;             /* 0x1c */
	int mSpace_width;          /* 0x20 */
	                           /* sizeof(FontInfo) == 0x24 */
};

/* one string queued by Font_String / Font_Number, drawn by Font_Display */
struct SString {
	int End;       /* 0x00 */
	ushort X;      /* 0x04 */
	ushort Y;      /* 0x06 */
	char *pString; /* 0x08 */
	uint CodeBGR;  /* 0x0c */
	uchar Is3d;    /* 0x10 */
	uchar BigText; /* 0x11 */
	CVector Pos;   /* 0x14 */
	               /* sizeof(SString) == 0x20 */
};

/* functions */
void Font_Init();
void Font_Reset();
void Font_String(ushort X, ushort Y, char *pString, uchar r, uchar g, uchar b, uchar BigText);
void Font_Number(ushort X, ushort Y, int Number, uchar r, uchar g, uchar b, uchar BigText);
void Font_Display();
#endif
