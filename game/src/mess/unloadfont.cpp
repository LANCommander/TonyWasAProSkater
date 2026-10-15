/* Mess_UnloadFont__Fv (0x8001ef60) */
#include "game/font.h"

extern FontInfo Font_table[];

extern int Current_font, Num_fonts;

void Pack_Unpack(short, short);
void Free16Slot(unsigned short);
void Mem_Delete(void *);

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
