/* Load8BitBMP2__FPcPPcPiT2PUs (0x800613cc) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} RECT;

typedef struct {
	char _0[18];
	unsigned short w;
	char _14[2];
	unsigned short h;
	char _18[4];
	unsigned short bpp;
	char _1e[16];
	unsigned short ncol;
	char _30[6];
	unsigned char pal[1];
} BMP;

extern int Pal256X, Pal256Y;

int FUN_8008c190(char *, ...);
int GetFree256Slot(void);
int FUN_8008c714(RECT *, unsigned short *);
int FUN_8008c468(int);
char *Mem_NewX(int, int, int);
void Mem_Delete(void *);
unsigned short FUN_8008f70c(int, int);

int Load8BitBMP2(char *file, char **out, int *pw, int *ph, unsigned short *pclut) {
	BMP *b = (BMP *)file;
	unsigned short pal[256];
	RECT rc;
	int bpp, w, h, n, i, pad, slot, x, y, trans, row, col, k;
	unsigned char *src, *dst, *pp;
	unsigned short *pd;

	*out = 0;
	w = b->w;
	h = b->h;
	bpp = b->bpp;

	if (bpp != 8) {
		FUN_8008c190("\nNot 8 bits per pixel, is %d bits per pixel", bpp);
		return 0;
	}

	n = b->ncol;

	if (n == 0)
		n = 256;

	trans = 0;
	pp = b->pal;

	for (i = 0; i < n; i++) {
		unsigned short cr, cg, cb;

		cb = pp[0];
		cg = pp[1];
		cr = pp[2];

		if (cb >= 248 && cg == 0 && cr >= 248) {
			trans = i;
			pal[i] = 0;
		} else {
			cb >>= 3;
			cg >>= 3;
			cr >>= 3;

			if (cb == 0 && cg == 0 && cr == 0)
				pal[i] = 0x8000;
			else
				pal[i] = cr + ((cb << 10) + (cg << 5));
		}

		pp += 4;
	}

	x = Pal256X;
	slot = GetFree256Slot();
	y = Pal256Y + slot;
	rc.x = x;
	rc.y = y;
	rc.w = 256;
	rc.h = 1;
	FUN_8008c714(&rc, pal);
	FUN_8008c468(0);

	pad = w % 4;

	if (pad > 0)
		pad = 4 - pad;

	w += pad;
	*out = Mem_NewX(w * h, 1, 1);
	src = b->pal + n * 4;
	dst = (unsigned char *)*out + (h - 1) * w;

	for (row = 0; row < h; row++) {
		for (col = 0; col < w - pad; col++)
			*dst++ = *src++;

		for (k = 0; k < pad; k++) {
			*dst++ = trans;
			src++;
		}

		dst -= w * 2;
	}

	Mem_Delete(file);

	*pw = w;
	*ph = h;
	*pclut = FUN_8008f70c(x, y);
	return 1;
}
