/* Pal_Init__Fv (0x8004aa6c) */
#include "names.h"

extern void *DAT_800d1a80;
extern char Pal256X[4];
extern char Pal256Y[4];
extern char Pal16X[4];
extern char Pal16Y[4];

extern char Free16Slots[];
extern char Free256Slots[];

void Pack_Pack(int a, int b, int c, char *d, char *e, int f, int g, int h);

void Pal_Init(void) {
	int i;
	volatile int pad[2];

	DAT_800d1a80 = 0;
	Pack_Pack(-1, 256, 75, Pal256X, Pal256Y, 16, 0, 0);
	Pack_Pack(-1, 16, 256, Pal16X, Pal16Y, 16, 0, 0);

	for (i = 0; i < 256; i++)
		Free16Slots[i] = 1;

	Free256Slots[0] = 0;

	for (i = 1; i < 75; i++)
		Free256Slots[i] = 1;
}
