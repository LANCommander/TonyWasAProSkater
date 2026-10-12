/* Display__6CamPosUsUsUcUcUc (0x8007d258) */
/* C++ (CAMERA.CPP). Members from the demo's MAIN.SYM (the calc_/lock_ flags have no type there). */
#include "game/camera.h"

void Font_String(ushort, ushort, char *, uchar, uchar, uchar, uchar);
void Font_Number(ushort, ushort, int, uchar, uchar, uchar, uchar);

extern char s_Follow[];
extern char s_Fixed[];
extern char s_quaternion[];

void CamPos::Display(ushort x, ushort y, uchar r, uchar g, uchar b) {
	if (calc_mTripod) {
		Font_String(x, y, s_Follow, r, g, b, 0);
		Font_Number(x, y + 8, mTripod->mPos.vx >> 12, r, g, b, 0);
		Font_Number(x, y + 16, mTripod->mPos.vy >> 12, r, g, b, 0);
		Font_Number(x, y + 24, mTripod->mPos.vz >> 12, r, g, b, 0);
	} else {
		Font_String(x, y, s_Fixed, r, g, b, 0);
		Font_Number(x, y + 8, mTripodPos.vx >> 12, r, g, b, 0);
		Font_Number(x, y + 16, mTripodPos.vy >> 12, r, g, b, 0);
		Font_Number(x, y + 24, mTripodPos.vz >> 12, r, g, b, 0);
	}

	Font_String(x, y + 40, s_quaternion, r, g, b, 0);
	Font_Number(x, y + 48, mStickLength, r, g, b, 0);
	Font_Number(x, y + 64, mStickOffset.vx >> 12, r, g, b, 0);
	Font_Number(x, y + 72, mStickOffset.vy >> 12, r, g, b, 0);
	Font_Number(x, y + 80, mStickOffset.vz >> 12, r, g, b, 0);
}
