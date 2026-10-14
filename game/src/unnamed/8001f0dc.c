/* Mess_UnloadAllFonts__Fv (0x8001f0dc) */
#include "names.h"

struct S {
	int a;
	char r[32];
};

extern struct S Font_table_14[];

int Current_font;

void Mess_UnloadFont(void);

void Mess_UnloadAllFonts(void) {
	int i;

	for (i = 0; i < 10; i++) {
		if (Font_table_14[i].a) {
			Current_font = i;
			Mess_UnloadFont();
		}
	}
}
