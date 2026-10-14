/* FUN_80061c98 (0x80061c98) */
#include "names.h"
#ifdef PC_PORT
#include "pc_texstore.h"
#endif

int Mem_NewX(int a, int b, int c);
void StoreImage(short *r, int b);
void LoadImage(short *r, int b);
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
#ifdef PC_PORT
	PC_TexRead(r[0], r[1], r[2], r[3], (void *)buf);
#else
	StoreImage(r, buf);
#endif

	q = (unsigned short *)buf;

	for (i = 0; i < 256; i++) {
		if (*q)
			*q |= 32768;

		q++;
	}

#ifdef PC_PORT
	PC_TexUpload(r[0], r[1], r[2], r[3], (void *)buf);
#else
	LoadImage(r, buf);
#endif
	Mem_Delete(buf);
}
