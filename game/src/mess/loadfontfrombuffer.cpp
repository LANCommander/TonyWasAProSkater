/* Mess_LoadFont__FPcT0iii (0x8001e8c8) */
/* C++ (MESS.CPP). Locals, their order and types come from the demo's MAIN.SYM; statements follow its line table. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/font.h"
#include "game/psyq.h"
#include "pc_texstore.h"

extern FontInfo Font_table[10];

int Num_fonts, Current_font;
u_short Clut;

extern int Pack_NoBottom, Pack_NoRight, Pal16X, Pal16Y;

void Mess_SetNewFont(u_char);
void *Mem_New(u_int, int, int);
void Mem_Delete(void *);
void Pack_Pack(int, int, int, int *, int *, int, int, u_int);
int GetFree16Slot(void);

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
