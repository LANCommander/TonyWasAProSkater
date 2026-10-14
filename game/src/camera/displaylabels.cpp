/* DisplayLabels__6CamPosUsUsUcUcUc (0x8007d4b4) */
/* C++ (CAMERA.CPP). Debug overlay: the field labels printed next to CamPos::Display's values (a static
   member: it reads no camera state). */
#include "game/camera.h"

void Font_String(ushort x, ushort y, char *s, uchar r, uchar g, uchar b, uchar a);

void CamPos::DisplayLabels(ushort x, ushort y, uchar r, uchar g, uchar b) {
	ushort x2;

	Font_String(x, y, "Tripod", r, g, b, 0);

	x2 = x + 20;
	Font_String(x2, y + 8, "x", r, g, b, 0);
	Font_String(x2, y + 16, "y", r, g, b, 0);
	Font_String(x2, y + 24, "z", r, g, b, 0);
	Font_String(x, y + 40, "Stick O", r, g, b, 0);
	Font_String(x, y + 48, "Stick Length", r, g, b, 0);
	Font_String(x, y + 56, "Stick Offset", r, g, b, 0);
	Font_String(x2, y + 64, "x", r, g, b, 0);
	Font_String(x2, y + 72, "y", r, g, b, 0);
	Font_String(x2, y + 80, "z", r, g, b, 0);
}
