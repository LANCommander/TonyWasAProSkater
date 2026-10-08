/* DisplayLabels__6CamPosUsUsUcUcUc (0x8007d4b4) */
#include "names.h"
void Font_String(unsigned short x, unsigned short y, char *s, unsigned char a,
                                  unsigned char b, unsigned char c, unsigned char d);

extern char s_Tripod[], DAT_800d20b0[], DAT_800d20b4[], DAT_800d20b8[], s_Stick_O[],
    s_Stick_Length[], s_Stick_Offset[];

void CamPos_DisplayLabels(unsigned short x, unsigned short y, unsigned char r,
                                      unsigned char g, unsigned char b) {
	unsigned short x2;

	Font_String(x, y, s_Tripod, r, g, b, 0);

	x2 = x + 20;
	Font_String(x2, y + 8, DAT_800d20b0, r, g, b, 0);
	Font_String(x2, y + 16, DAT_800d20b4, r, g, b, 0);
	Font_String(x2, y + 24, DAT_800d20b8, r, g, b, 0);
	Font_String(x, y + 40, s_Stick_O, r, g, b, 0);
	Font_String(x, y + 48, s_Stick_Length, r, g, b, 0);
	Font_String(x, y + 56, s_Stick_Offset, r, g, b, 0);
	Font_String(x2, y + 64, DAT_800d20b0, r, g, b, 0);
	Font_String(x2, y + 72, DAT_800d20b4, r, g, b, 0);
	Font_String(x2, y + 80, DAT_800d20b8, r, g, b, 0);
}
