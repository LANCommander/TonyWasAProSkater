/* Mess_SetFontScale__FUi (0x8001f1d8) */
#include "names.h"

typedef struct {
	char pad[0x1a];
	unsigned short scale;
	char pad2[8];
} Font;

unsigned char NewFont;
int Current_font;

extern Font Font_table[];

extern void Mess_SetScale(int);

void Mess_SetFontScale(unsigned s) {
	if (!NewFont)
		Mess_SetScale(s);
	else
		(Current_font + Font_table)->scale = s;
}
