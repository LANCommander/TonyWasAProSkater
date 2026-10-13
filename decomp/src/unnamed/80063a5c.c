/* FUN_80063a5c (0x80063a5c) */
#include "names.h"

extern char __vt_6Sprite[];

int Load8BitBMP2(char *name, char **a, int *b, int *c, unsigned short *d);
char *__nw__6CClassUi(unsigned n);
char *SlicedImage_ctor(char *p, char *a, int b, int c, int d, int e, int f, int g);
int FUN_800627ec(char *p, int a, int b);

char *FUN_80063a5c(char *p, int f16, char *name, int a3, int x, int mw, int mh) {
	unsigned short pal;
	char *data;
	int w;
	int h;
	char *v;

	*(char **)p = __vt_6Sprite;
	Load8BitBMP2(name, &data, &w, &h, &pal);

	*(int *)(p + 16) = f16;
	*(int *)(p + 4) = w;
	*(int *)(p + 8) = h;

	if (!(mw * 2 < w))
		mw = 0;

	if (!(mh < h))
		mh = 0;

	v = __nw__6CClassUi(52);

	if (v)
		v = SlicedImage_ctor(v, data, w, h, mw, mh, *(int *)(p + 16), pal);

	*(char **)(p + 20) = v;
	data = 0;
	*(int *)(p + 12) = 1;
	FUN_800627ec(v, a3, x);
	return p;
}
