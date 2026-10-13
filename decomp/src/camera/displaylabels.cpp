/* DisplayLabels__6CamPosUsUsUcUcUc (0x8007d4b4) */
/* C++ (CAMERA.CPP). Debug overlay: the field labels printed next to CamPos::Display's values (a static
   member: it reads no camera state). */
#include "game/camera.h"

void Font_String(ushort x, ushort y, char *s, uchar r, uchar g, uchar b, uchar a);

extern char s_Tripod[], DAT_800d20b0[], DAT_800d20b4[], DAT_800d20b8[], s_Stick_O[],
    s_Stick_Length[], s_Stick_Offset[];

void CamPos::DisplayLabels(ushort x, ushort y, uchar r, uchar g, uchar b) {
	ushort x2;

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
