/* Bang__FUcUcUc (0x8004ce28) */
#include "names.h"
#ifdef PC_PORT
#include "pc_screen.h"
#endif

char DAT_800d1b38[8];

void ClearImage(char *, int, int, int);

void Bang(unsigned char a, unsigned char b, unsigned char c) {
#ifdef PC_PORT
	{
		short *r = (short *)DAT_800d1b38;

		PC_ScreenFill(r[0], r[1], r[2], r[3], a | b << 8 | c << 16);
	}
#else
	ClearImage(DAT_800d1b38, a, b, c);
#endif

	for (;;)
		;
}
