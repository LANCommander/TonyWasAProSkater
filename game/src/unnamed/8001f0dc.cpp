/* Mess_UnloadAllFonts__Fv (0x8001f0dc) */
#include "game/font.h"

extern FontInfo Font_table[];

int Current_font;

void Mess_UnloadFont();

void Mess_UnloadAllFonts() {
	int i;

	for (i = 0; i < 10; i++) {
		if (Font_table[i].mChar_table) {
			Current_font = i;
			Mess_UnloadFont();
		}
	}
}
