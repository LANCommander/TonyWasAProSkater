/* FUN_80011214 (0x80011214) */
#include "names.h"

void Mess_SetRGB(unsigned char r, unsigned char g, unsigned char b, int a);

void FUN_80011214(void) {
	Mess_SetRGB(96, 96, 96, 0);
}
