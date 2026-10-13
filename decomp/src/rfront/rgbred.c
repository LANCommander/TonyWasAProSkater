/* RGBRed__Fv (0x800111e8) */
#include "names.h"

void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);

void RGBRed(void) {
	Mess_SetRGB(96, 96, 96, 0);
}
