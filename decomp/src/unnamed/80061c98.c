/* FUN_80061c98 (0x80061c98) */
#include "names.h"

int Mem_NewX(int a, int b, int c);
void FUN_8008c774(short *r, int b);
void FUN_8008c714(short *r, int b);
void Mem_Delete(int a);

void FUN_80061c98(int a, int b) {
	short r[4];
	int buf = Mem_NewX(512, 1, 1);
	unsigned short *q;
	int i;

	r[0] = a;
	r[1] = b;
	r[2] = 256;
	r[3] = 1;
	FUN_8008c774(r, buf);

	q = (unsigned short *)buf;

	for (i = 0; i < 256; i++) {
		if (*q)
			*q |= 32768;

		q++;
	}

	FUN_8008c714(r, buf);
	Mem_Delete(buf);
}
