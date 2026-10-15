/* Mess_SetCurrentFont__FPc (0x8001f144) */

int Num_fonts;
int Current_font;

#include "game/font.h"

extern FontInfo Font_table[];

#include "game/psyq.h"

void Mess_SetCurrentFont(char *name) {
	int i;

	for (i = 0; i < Num_fonts; i++) {
		if (!strcmp(name, Font_table[i].mFont_name))
			break;
	}

	Current_font = i;
}
