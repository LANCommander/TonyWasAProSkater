/* SHELLUTILS.CPP: the front end's utility classes: 8-bit BMP loading, SlicedImage, Sprite,
   EraseSprite, PadBuffer, VerticalMenu / VerticalDialog, FileLoader and PREManager's constructor,
   destructor and loadPRE (declarations in game/shellutils.h and game/pre.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "pc_texstore.h"
#include "game/shellutils.h"
#include "game/psyq.h"
#include "game/types.h"
#include "pc_assets.h"
#include "pc_frame.h"
#include "pc_screen.h"
#include "game/dbuffer.h"
#include "game/m3d.h"
#include "pc_draw.h"
#include "game/pad.h"
#include "game/mess.h"
#include "game/fileio.h"
#include "game/spool.h"
#include "game/pre.h"
#include "game/mem.h"
#include "game/globals.h"
#include "game/pack.h"
#include "game/pal.h"

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
int LoadImage(RECT *, unsigned short *);
unsigned short FUN_8008f70c(int, int);
extern "C" void FUN_80061a10(char *);
void StoreImage(short *r, int b);
void LoadImage(short *r, int b);
extern int DAT_800d1d98;
extern "C" char __vt_11SlicedImage[];
/* egcs-ABI call, as retail makes it */
extern "C" void __dl__6CClassPv(void *p);
extern "C" void FUN_80061c98(int a, int b);
void DrawIt(RECT *r, int x, int y, int bpp, int clut);
int Load8BitBMP(char *, char **, int *, int *, ushort *);
extern "C" char __vt_6Sprite[];
extern "C" void FUN_800627ec(SlicedImage *p, int a, int b);
extern int DAT_800d1d9c;
extern char __vt_14VerticalDialog[];
/* VerticalMenu::VerticalMenu, egcs-ABI call (the C-source VerticalDialog constructor below) */
extern "C" int __12VerticalMenuiiiPPciPiT6i(char **p, int a, int b, int c, char **d, int e, int *f,
                                           int *g, int h);
extern char DAT_800a2130[];
struct PREManager;
extern "C" void FUN_8006582c(FileLoader *p);
extern "C" void FUN_80065d80(FileLoader *p);
extern char DAT_800a2120[];

/* Load8BitBMP2__FPcPPcPiT2PUs (0x800613cc) */
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
		printf("\nNot 8 bits per pixel, is %d bits per pixel", bpp);
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
	PC_TexUpload(rc.x, rc.y, rc.w, rc.h, pal);
	DrawSync(0);

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

/* Load8BitBMP__FPcPPcPiT2PUs (0x80061654) */
int Load8BitBMP(char *name, char **a, int *b, int *c, unsigned short *d) {
	int r = FileIO_Open(name);
	char *p;

	if (r <= 0) {
		printf("\nFile %s not found", name);
		return 0;
	}

	p = Mem_NewX(r, 1, 1);

	if (p) {
		FileIO_Load(p);
		FileIO_Sync();
		Load8BitBMP2(p, a, b, c, d);
		return 1;
	}

	printf("\nNot enough memory to load %s", name);
	return 0;
}

/* DownloadBMP16x2__FPc (0x80061714) -- 8-bit BMP to 16bpp at VRAM (0,0), line-doubled; variant of DownloadBMP__FPciiii */
void DownloadBMP16x2(char *pData) {
	int w;
	int h;
	int colors;
	int transparent_color;
	ushort colorTab[256];
	char *pColor;
	RECT r;
	int pad;
	char *data;
	char *in;
	char *out;

	/* a Dreamcast screen, already 640x480 (pc_assets.h): loaded as it is, not line-doubled
	   (DownloadBMP16, 0x80061a10) */
	{
		int lw, lh;

		if (PC_BmpLayoutSize(pData, &lw, &lh) && lh == *(ushort *)(pData + 22)) {
			FUN_80061a10(pData);
			PC_FrameWait(1);
			return;
		}
	}

	if (pBMR) {
		Mem_Delete(pBMR);

		pBMR = 0;
	}

	w = *(ushort *)(pData + 18);
	h = *(ushort *)(pData + 22);

	if (*(ushort *)(pData + 28) != 8) {
		printf("\nNot 8 bits per pixel");
		return;
	}

	transparent_color = 0;
	colors = *(ushort *)(pData + 46);

	if (colors == 0)
		colors = 256;

	pColor = pData + 54;

	{
		int i;

		for (i = 0; i < colors; i++) {
			ushort r, g, b;

			b = (unsigned char)pColor[0];
			g = (unsigned char)pColor[1];
			r = (unsigned char)pColor[2];

			if (b >= 248 && g == 0 && r >= 248) {
				transparent_color = i;
				colorTab[i] = 0;
			} else {
				b >>= 3;
				g >>= 3;
				r >>= 3;

				if (b == 0 && g == 0 && r == 0)
					colorTab[i] = 0x8000;
				else
					colorTab[i] = (b << 10) + (g << 5) + r;
			}

			pColor += 4;
		}
	}

	pad = w % 4;

	if (pad > 0)
		pad = 4 - pad;

	w += pad;
	data = (char *)Mem_New(w * h * 4, 1, 1);
	in = pData + 54 + colors * 4;
	out = data + (h * 2 - 1) * w;

	{
		int j;

		for (j = 0; j < h; j++) {
			{
				int i;

				for (i = 0; i < w - pad; i++)
					*out++ = *in++;
			}

			{
				int i;

				for (i = 0; i < pad; i++) {
					*out++ = transparent_color;
					in++;
				}
			}

			out -= w * 2;
			in -= w;

			{
				int i;

				for (i = 0; i < w - pad; i++)
					*out++ = *in++;
			}

			{
				int i;

				for (i = 0; i < pad; i++) {
					*out++ = transparent_color;
					in++;
				}
			}

			out -= w * 2;
		}
	}

	Mem_Delete(pData);

	h *= 2;

	{
		int n = w * h;
		ushort *dst = (ushort *)data + n - 1;
		int i;

		in = data + n - 1;
		i = n;

		while (i-- > 0)
			*dst-- = colorTab[(unsigned char)*in--];
	}

	r.x = 0;
	r.y = 0;
	r.w = w;
	r.h = h;
	DrawSync(0);
	PC_ScreenBlit(r.x, r.y, r.w, r.h, data);
	DrawSync(0);
	PC_FrameWait(1);
	Mem_Delete(data);
}

/* DownloadBMP16__FPc (0x80061a10) -- 8-bit BMP to 16bpp at VRAM (0,0); variant of DownloadBMP__FPciiii */
void DownloadBMP16(char *pData) {
	int w;
	int h;
	int colors;
	int transparent_color;
	ushort colorTab[256];
	char *pColor;
	RECT r;
	int pad;
	char *data;
	char *in;
	char *out;
	/* a Dreamcast image bigger than the screen it fills (pc_assets.h): kept whole */
	int lw = 0;
	int lh = 0;
	int hires = PC_BmpLayoutSize(pData, &lw, &lh) &&
	            (lw != *(ushort *)(pData + 18) || lh != *(ushort *)(pData + 22));

	if (pBMR) {
		Mem_Delete(pBMR);

		pBMR = 0;
	}

	w = *(ushort *)(pData + 18);
	h = *(ushort *)(pData + 22);

	if (*(ushort *)(pData + 28) != 8) {
		printf("\nNot 8 bits per pixel");
		return;
	}

	transparent_color = 0;
	colors = *(ushort *)(pData + 46);

	if (colors == 0)
		colors = 256;

	pColor = pData + 54;

	{
		int i;

		for (i = 0; i < colors; i++) {
			ushort r, g, b;

			b = (unsigned char)pColor[0];
			g = (unsigned char)pColor[1];
			r = (unsigned char)pColor[2];

			if (b >= 248 && g == 0 && r >= 248) {
				transparent_color = i;
				colorTab[i] = 0;
			} else {
				b >>= 3;
				g >>= 3;
				r >>= 3;

				if (b == 0 && g == 0 && r == 0)
					colorTab[i] = 0x8000;
				else
					colorTab[i] = (b << 10) + (g << 5) + r;
			}

			pColor += 4;
		}
	}

	pad = w % 4;

	if (pad > 0)
		pad = 4 - pad;

	w += pad;
	data = (char *)Mem_New(w * h * 2, 1, 1);
	in = pData + 54 + colors * 4;
	out = data + (h - 1) * w;

	{
		int j;

		for (j = 0; j < h; j++) {
			{
				int i;

				for (i = 0; i < w - pad; i++)
					*out++ = *in++;
			}

			{
				int i;

				for (i = 0; i < pad; i++) {
					*out++ = transparent_color;
					in++;
				}
			}

			out -= w * 2;
		}
	}

	Mem_Delete(pData);

	{
		int n = w * h;
		ushort *dst = (ushort *)data + n - 1;
		int i;

		in = data + n - 1;
		i = n;

		while (i-- > 0)
			*dst-- = colorTab[(unsigned char)*in--];
	}

	r.x = 0;
	r.y = 0;
	r.w = w;
	r.h = h;
	if (hires)
		PC_ScreenBlitHiRes(0, 0, lw, lh, data, w, h);
	else
		PC_ScreenBlit(r.x, r.y, r.w, r.h, data);

	DrawSync(0);
	Mem_Delete(data);
}

/* FUN_80061c98 (0x80061c98) */
void FUN_80061c98(int a, int b) {
	short r[4];
	void *buf = Mem_NewX(512, 1, 1);
	unsigned short *q;
	int i;

	r[0] = a;
	r[1] = b;
	r[2] = 256;
	r[3] = 1;
	PC_TexRead(r[0], r[1], r[2], r[3], buf);

	q = buf;

	for (i = 0; i < 256; i++) {
		if (*q)
			*q |= 32768;

		q++;
	}

	PC_TexUpload(r[0], r[1], r[2], r[3], buf);
	Mem_Delete(buf);
}

/* DrawColoredBox__FiiiiUcUcUc (0x80061d44) */
void DrawColoredBox(int x, int y, int w, int h, u_char r, u_char g, u_char b) {
	POLY_F4 *Poly = (POLY_F4 *)pPoly;

	pPoly = (u_char *)(Poly + 1);
	setPolyF4(Poly);
	setRGB0(Poly, r, g, b);
	setXY4(Poly, x, y, x + w, y, x, y + h, x + w, y + h);
	addPrim(&pDoubleBuffer->OT3D[OT_entry], Poly);

	OT_entry--;
}

/* SetClippingBox__Fiiii (0x80061e00) */
void SetClippingBox(int x, int y, int w, int h) {
	RECT r;
	DR_AREA *pClip = (DR_AREA *)pPoly;

	setRECT(&r, x, y, w, h);
	SetDrawArea(pClip, &r);

	pPoly = (u_char *)(pClip + 1);
	addPrim(&pDoubleBuffer->OT3D[OT_entry], pClip);

	OT_entry--;
}

/* ResetOT__Fv (0x80061ea0) */
void ResetOT(void) {
	DAT_800d1d98 = 0;
	OT_entry = 4095;
}

/* SetOTPlayer__Fi (0x80061eb4) */
void SetOTPlayer(int a) {
	if (DAT_800d1d98 != a) {
		if (a == 0)
			OT_entry -= 4096;
		else
			OT_entry += 4096;

		DAT_800d1d98 = a;
	}
}

/* __11SlicedImagePciiiiiUs (0x80061ef4) */
/* Copies a w x h image (8-bit images count 16-bit words, so half width) into
   a buffer laid out slice column by slice column (slices of sw x sh; 0 = whole image), plus a
   narrower last column for the remainder, then frees the source pixels. */
SlicedImage::SlicedImage(char *pPixels, int w, int h, int sw, int sh, int depth, ushort clutid) {
	ushort *d, *s;
	int rest;

	is_packed = 0;
	bpp = depth;
	clut = clutid;

	if (depth == 8)
		w = w >> 1;

	if (sw == 0)
		sw = w;

	if (sh == 0)
		sh = h;

	width = w;
	height = h;
	slice_width = sw;
	slice_height = sh;
	sliceTab = 0;
	num_slices_x = w / sw;
	num_slices_y = (h + sh - 1) / sh;
	slicedBuffer = (ushort *)Mem_NewX(w * h * 2, 1, 1);

	d = slicedBuffer;
	s = (ushort *)pPixels;

	for (int i = 0; i < num_slices_x; i++) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < slice_width; k++)
				*d++ = *s++;

			s += width - slice_width;
		}

		s += slice_width - width * height;
	}

	rest = width - num_slices_x * slice_width;

	if (rest > 0) {
		for (int j = 0; j < height; j++) {
			for (int k = 0; k < rest; k++)
				*d++ = *s++;

			s += width - rest;
		}

		num_slices_x++;
	}

	Mem_Delete(pPixels);

	m_removed_from_memory = 0;
}

/* FUN_800620f0 (0x800620f0) */
/* SlicedImage's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: frees its VRAM (packed slices, or its own area) and 256-colour CLUT slot, and its pixels
   unless they were already removed from memory; then the inlined ~CClass. */
extern "C" void FUN_800620f0(SlicedImage *p, int f) {
	p->__vptr = __vt_11SlicedImage;

	if (p->is_packed) {
		p->unpack();
	} else if (p->sliceTab) {
		Pack_Unpack(p->sliceTab->x, p->sliceTab->y);
		Mem_Delete(p->sliceTab);
	}

	if (p->bpp == 8)
		Free256Slot(p->clut);

	if (!p->m_removed_from_memory)
		Mem_Delete(p->slicedBuffer);

	p->__vptr = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* pack__11SlicedImage (0x800621c0) */
/* Locals from the demo's MAIN.SYM, plus numx, which retail keeps in s5. */
int SlicedImage::pack() {
	int j;
	int k;
	ushort *data_col;
	ushort *data;
	RECT r;
	int remx;
	int numy;
	int remy;
	int dum1;
	int dum2;
	int numx;

	if (is_packed)
		return 0;

	is_packed = 1;
	sliceTab = (RECT *)Mem_New(num_slices_y * num_slices_x * sizeof(RECT), 1, 1);
	data_col = slicedBuffer;
	numx = width / slice_width;
	remx = width - numx * slice_width;
	numy = height / slice_height;
	remy = height - numy * slice_height;
	Pack_NoBottom = 1;

	for (j = 0; j < numx; j++) {
		data = data_col;

		for (k = 0; k < numy; k++) {
			if (bpp == 8)
				Pack_Pack(0, slice_width * 2, slice_height, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, slice_width, slice_height, &dum1, &dum2, bpp, 0, 0);

			r.w = slice_width;
			r.h = slice_height;
			r.x = dum1;
			r.y = dum2;
			sliceTab[k * num_slices_x + j].x = dum1;
			sliceTab[k * num_slices_x + j].y = dum2;
			sliceTab[k * num_slices_x + j].w = slice_width;
			sliceTab[k * num_slices_x + j].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += slice_width * slice_height;
		}

		if (remy > 0) {
			if (bpp == 8)
				Pack_Pack(0, slice_width * 2, remy, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, slice_width, remy, &dum1, &dum2, bpp, 0, 0);

			r.h = remy;
			r.w = slice_width;
			r.x = dum1;
			r.y = dum2;
			sliceTab[numy * num_slices_x + j].x = dum1;
			sliceTab[numy * num_slices_x + j].y = dum2;
			sliceTab[numy * num_slices_x + j].w = slice_width;
			sliceTab[numy * num_slices_x + j].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}

		data_col += slice_width * height;
	}

	if (remx > 0) {
		data = data_col;

		for (k = 0; k < numy; k++) {
			if (bpp == 8)
				Pack_Pack(0, remx * 2, slice_height, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, remx, slice_height, &dum1, &dum2, bpp, 0, 0);

			r.w = remx;
			r.h = slice_height;
			r.x = dum1;
			r.y = dum2;
			sliceTab[k * num_slices_x + numx].x = dum1;
			sliceTab[k * num_slices_x + numx].y = dum2;
			sliceTab[k * num_slices_x + numx].w = remx;
			sliceTab[k * num_slices_x + numx].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += remx * slice_height;
		}

		if (remy > 0) {
			if (bpp == 8)
				Pack_Pack(0, remx * 2, remy, &dum1, &dum2, bpp, 0, 0);
			else if (bpp == 16)
				Pack_Pack(0, remx, remy, &dum1, &dum2, bpp, 0, 0);

			r.w = remx;
			r.h = remy;
			r.x = dum1;
			r.y = dum2;
			sliceTab[numy * num_slices_x + numx].x = dum1;
			sliceTab[numy * num_slices_x + numx].y = dum2;
			sliceTab[numy * num_slices_x + numx].w = remx;
			sliceTab[numy * num_slices_x + numx].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}
	}

	Pack_NoBottom = 0;
	return DrawSync(0);
}

/* packAt__11SlicedImageii (0x800627ec) */
/* SlicedImage method between pack and unpack that THPS1 added (the demo has
   nothing at SHELLUTILS.CPP lines 688-831): loads the slices to a fixed VRAM position (x, y) instead
   of Pack_Pack-allocated space, column by column, then frees the source buffer
   (FUN_80062de8 = removeFromMemory). is_packed stays 0, so unpack won't release the VRAM.
   Codegen notes: sx/sy are function-scope copies assigned in every block (a variable set in
   several blocks becomes CSE's canonical register, so the copies survive: retail's s4/t0);
   curx is declared before remx and remy at its assignment, which fixes the spill-slot order. */
void SlicedImage::packAt(int x, int y) {
	int j;
	int k;
	ushort *data_col;
	ushort *data;
	RECT r;
	int curx;
	int remx;
	int numy;
	int numx;
	int cury;
	int sx;
	int sy;

	is_packed = 0;
	sliceTab = (RECT *)Mem_New(num_slices_y * num_slices_x * sizeof(RECT), 1, 1);
	data_col = slicedBuffer;
	numx = width / slice_width;
	numy = height / slice_height;
	remx = width - numx * slice_width;
	int remy = height - numy * slice_height;

	curx = x;
	Pack_NoBottom = 1;

	for (j = 0; j < numx; j++) {
		data = data_col;
		cury = y;

		for (k = 0; k < numy; k++) {
			sx = curx;
			sy = cury;
			cury = sy + slice_height;
			r.x = sx;
			r.y = sy;
			r.w = slice_width;
			r.h = slice_height;
			sliceTab[k * num_slices_x + j].x = sx;
			sliceTab[k * num_slices_x + j].y = sy;
			sliceTab[k * num_slices_x + j].w = slice_width;
			sliceTab[k * num_slices_x + j].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += slice_width * slice_height;
		}

		if (remy > 0) {
			sx = curx;
			sy = cury;
			r.h = remy;
			r.y = sy;
			r.x = sx;
			r.w = slice_width;
			sliceTab[numy * num_slices_x + j].x = sx;
			sliceTab[numy * num_slices_x + j].y = sy;
			sliceTab[numy * num_slices_x + j].w = slice_width;
			sliceTab[numy * num_slices_x + j].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}

		curx += slice_width;
		data_col += slice_width * height;
	}

	if (remx > 0) {
		data = data_col;
		cury = y;

		for (k = 0; k < numy; k++) {
			sx = curx;
			sy = cury;
			cury = sy + slice_height;
			r.x = sx;
			r.y = sy;
			r.w = remx;
			r.h = slice_height;
			sliceTab[k * num_slices_x + numx].x = sx;
			sliceTab[k * num_slices_x + numx].y = sy;
			sliceTab[k * num_slices_x + numx].w = remx;
			sliceTab[k * num_slices_x + numx].h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);

			data += remx * slice_height;
		}

		if (remy > 0) {
			sx = curx;
			sy = cury;
			r.w = remx;
			r.y = sy;
			r.x = sx;
			r.h = remy;
			sliceTab[numy * num_slices_x + numx].x = sx;
			sliceTab[numy * num_slices_x + numx].y = sy;
			sliceTab[numy * num_slices_x + numx].w = remx;
			sliceTab[numy * num_slices_x + numx].h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
		}
	}

	Pack_NoBottom = 0;
	DrawSync(0);
	removeFromMemory();
}

/* unpack__11SlicedImage (0x80062d00) */
/* Releases the image's VRAM: every slice if it was packed, else the single
   slice (if any); then frees the slice table. */
void SlicedImage::unpack() {
	if (!is_packed) {
		if (!sliceTab)
			return;

		Pack_Unpack(sliceTab->x, sliceTab->y);
	} else {
		is_packed = 0;

		for (int i = 0; i < num_slices_y; i++)
			for (int j = 0; j < num_slices_x; j++)
				Pack_Unpack(sliceTab[i * num_slices_x + j].x, sliceTab[i * num_slices_x + j].y);
	}

	Mem_Delete(sliceTab);

	sliceTab = 0;
}

/* removeFromMemory__11SlicedImage (0x80062de8) */
/* Frees the image's pixel copy in main RAM (once). */
void SlicedImage::removeFromMemory() {
	if (!m_removed_from_memory) {
		m_removed_from_memory = 1;
		Mem_Delete(slicedBuffer);
	}
}

/* draw__11SlicedImageiiiiiiii (0x80062e20) */
/* Emits one POLY_FT4 per slice of the image at (x, y), flat (r == -1) or
   tinted, optionally semi-transparent, into the 2D OT (otFront) or successive OT layers counting
   down from OT_entry, or draws it at once (drawNow). The tpage is getTPage on the slice RECT. */
void SlicedImage::draw(int x, int y, int otFront, int r, int g, int b, int semiTrans, int drawNow) {
	drawScaled(x, y, 0, 0, otFront, r, g, b, semiTrans, drawNow);
}

/* w x h: the size on screen (0 = the image's own; a Sprite of an image the Dreamcast redrew
   larger is laid out at the PS1's size, platform/pc/include/pc_assets.h). Slice edges are placed
   from their offsets in the image, so scaled slices still meet. */
void SlicedImage::drawScaled(int x, int y, int w, int h, int otFront, int r, int g, int b,
                             int semiTrans, int drawNow) {
	int slice = 0;
	int shift = bpp == 8;
	P_TAG *ot;

	if ((u_int)OT_entry >= 0x2000)
		printf("\nOT_entry is %d", OT_entry);

	if (otFront)
		ot = &pDoubleBuffer->OT2D;
	else
		ot = &pDoubleBuffer->OT3D[OT_entry];

	/* the image's size in pixels: its first row and column of slices */
	int iw = 0;
	int ih = 0;

	for (int j = 0; j < num_slices_x; j++)
		iw += sliceTab[j].w << shift;

	for (int i = 0; i < num_slices_y; i++)
		ih += sliceTab[i * num_slices_x].h;

	if (w <= 0 || h <= 0 || !iw || !ih) {
		w = iw;
		h = ih;
	}

	int oy = 0;

	for (int i = 0; i < num_slices_y; i++) {
		int ox = 0;
		int sh = sliceTab[i * num_slices_x].h;
		int y0 = y + oy * h / ih;
		int y1 = y + (oy + sh) * h / ih;

		for (int j = 0; j < num_slices_x; j++) {
			RECT *pSlice = &sliceTab[slice];
			POLY_FT4 *p = (POLY_FT4 *)pPoly;

			pPoly += sizeof(POLY_FT4);
			setlen(p, 9);

			p->code = 0x2c;

			if (r == -1) {
				p->r0 = 0;
				p->g0 = 0;
				p->b0 = 0;
			} else {
				p->r0 = r;
				p->g0 = g;
				p->b0 = b;
			}

			int x0 = x + ox * w / iw;
			int x1 = x + (ox + (pSlice->w << shift)) * w / iw;

			p->x0 = x0;
			p->y0 = y0;
			p->x1 = x1;
			p->y1 = y0;
			p->x2 = x0;
			p->y2 = y1;
			p->x3 = x1;
			p->y3 = y1;
			p->u0 = (pSlice->x & 63) << shift;
			p->v0 = pSlice->y;
			p->u1 = ((pSlice->x & 63) << shift) + (pSlice->w << shift);
			p->v1 = pSlice->y;
			p->u2 = (pSlice->x & 63) << shift;
			p->v2 = pSlice->h + pSlice->y;
			p->u3 = ((pSlice->x & 63) << shift) + (pSlice->w << shift);
			p->v3 = pSlice->h + pSlice->y;

			if (semiTrans)
				p->code |= 2;
			else
				p->code &= ~2;

			if (r == -1)
				p->code |= 1;
			else
				p->code &= ~1;

			p->tpage = getTPage(bpp == 8 ? 1 : 2, 0, pSlice->x, pSlice->y);
			p->clut = clut;

			if (!drawNow)
				addPrim(ot, p);
			else
				PC_DrawPacket((void *)(p));

			if (ot != &pDoubleBuffer->OT2D)
				OT_entry--;

			slice++;
			ox += pSlice->w << shift;
		}

		oy += sh;
	}
}

/* FUN_80063290 (0x80063290) */
/* C++: (re)loads a sliced image's 256-colour palette into its slot. */
extern "C" void FUN_80063290(SlicedImage *p) {
	int n = Get256SlotNumber(p->clut);

	FUN_80061c98(Pal256X, Pal256Y + n);
}

/* DrawIt__FP4RECTiiii (0x800632cc) */
void DrawIt(RECT *r, int x, int y, int bpp, int clut) {
	POLY_FT4 poly;
	POLY_FT4 *apoly;

	apoly = &poly;
	setPolyFT4(apoly);
	setRGB0(apoly, 0, 0, 0);
	setShadeTex(apoly, 1);

	if (bpp == 16) {
		setXYWH(apoly, x, y, r->w, r->h);
		setUVWH(apoly, r->x & 0x3f, r->y & 0xff, r->w, r->h);

		apoly->tpage = getTPage(2, 0, r->x, r->y);
	} else {
		setXYWH(apoly, x, y, r->w << 1, r->h);
		setUVWH(apoly, (r->x & 0x3f) << 1, r->y & 0xff, r->w << 1, r->h);

		apoly->tpage = getTPage(1, 0, r->x, r->y);
		apoly->clut = clut;
	}

	PC_DrawPacket((void *)(apoly));
	DrawSync(0);
}

/* rotateDraw__11SlicedImageii (0x80063474) */
/* Demo twin; names from its MAIN.SYM. Uploads the image one slice at a time
   into a single VRAM RECT (allocated on first use) and draws each slice rotated via DrawIt.
   numx/remy are declared mid-block, at their first assignment: their pseudos are then created
   after the division's quotient temp, which gives retail's spill-slot order (numx 48, remy 52). */
void SlicedImage::rotateDraw(int x, int y) {
	RECT *tab = sliceTab;
	RECT r;
	int dum1, dum2;
	int w, h, sw, sh;
	int numy, remx, j, k;
	ushort *data_col, *data;
	int xt, yt;

	if (!tab) {
		sliceTab = (RECT *)Mem_New(sizeof(RECT), 1, 1);
		Pack_Pack(0, slice_width, slice_height, &dum1, &dum2, 16, 0, 0);

		sliceTab->w = slice_width;
		sliceTab->h = slice_height;
		sliceTab->x = dum1;
		sliceTab->y = dum2;
		tab = sliceTab;
	}

	w = width;
	sw = slice_width;
	int numx = w / sw;

	h = height;
	sh = slice_height;
	numy = h / sh;
	r.h = tab->h;
	r.w = tab->w;
	r.x = tab->x;
	r.y = tab->y;
	data_col = slicedBuffer;
	xt = x;
	Pack_NoBottom = 1;
	remx = w - numx * sw;
	int remy = h - numy * sh;

	for (j = 0; j < numx; j++) {
		data = data_col;
		yt = y;

		for (k = 0; k < numy; k++) {
			r.w = slice_width;
			r.h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);

			yt += slice_height;
			data += slice_width * slice_height;
		}

		if (remy > 0) {
			r.w = slice_width;
			r.h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);
		}

		data_col += slice_width * height;

		if (bpp == 16)
			xt += slice_width;
		else
			xt += slice_width * 2;
	}

	if (remx > 0) {
		data = data_col;
		yt = y;

		for (k = 0; k < numy; k++) {
			r.w = remx;
			r.h = slice_height;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);

			yt += slice_height;
			data += remx * slice_height;
		}

		if (remy > 0) {
			r.w = remx;
			r.h = remy;
			PC_TexUpload(r.x, r.y, r.w, r.h, data);
			DrawSync(0);
			DrawIt(&r, xt, yt, bpp, clut);
		}
	}

	Pack_NoBottom = 0;
}

/* removeBlack__11SlicedImage (0x800637d0) */
/* In a 16-bit image, turns black (transparent) pixels into opaque black. */
void SlicedImage::removeBlack() {
	if (bpp == 16) {
		int n = width * height;
		ushort *p = slicedBuffer;

		for (int i = 0; i < n; i++, p++)
			if (!*p)
				*p = 0x8000;
	}
}

/* __6SpritePciii (0x80063830) */
/* An 8-bit sprite loaded from a BMP, sliced to at most maxw/2 x maxh (0 =
   whole image when the image is no bigger), optionally packed into VRAM at once. */
Sprite::Sprite(char *pName, int packed, int maxw, int maxh) {
	ushort clut;
	char *pData;
	int w;
	int h;

	Load8BitBMP(pName, &pData, &w, &h, &clut);

	m_bpp = 8;
	m_w = w;
	m_h = h;

	/* an image the Dreamcast redrew larger (pc_assets.h): laid out at the PS1's size, drawn
	   scaled to it with all its pixels (draw, drawFaded) */
	{
		int lw, lh;

		if (PC_ImageLayoutSize(pName, w, h, &lw, &lh)) {
			m_w = lw;
			m_h = lh;
		}
	}

	if (!(maxw * 2 < w))
		maxw = 0;

	if (!(maxh < h))
		maxh = 0;

	m_image = new SlicedImage(pData, w, h, maxw, maxh, m_bpp, clut);
	m_packed = packed;

	if (packed)
		m_image->pack();
}

/* __6Spriteiiiiiii (0x80063928) */
/* A 16-bit sprite grabbed from the VRAM rectangle (x, y, w, h), sliced to at
   most maxw x maxh (0 = whole), black made opaque, optionally packed into VRAM at once. */
Sprite::Sprite(int x, int y, int w, int h, int packed, int maxw, int maxh) {
	RECT rect;
	char *pData;

	m_bpp = 16;
	m_w = w;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	m_h = h;
	rect.h = h;

	if (maxw >= w)
		maxw = 0;

	if (maxh >= h)
		maxh = 0;

	pData = (char *)Mem_NewX(w * h * 2, 1, 1);
	PC_ScreenRead(rect.x, rect.y, rect.w, rect.h, pData);
	DrawSync(0);

	m_image = new SlicedImage(pData, w, h, maxw, maxh, 16, 0);
	m_image->removeBlack();

	m_packed = packed;

	if (packed)
		m_image->pack();
}

/* FUN_80063a5c (0x80063a5c) */
/* : a Sprite constructor, kept as the egcs-ABI function retail has: loads an
   8-bit BMP into a SlicedImage and loads that to VRAM at (a3, x). */
/* SlicedImage::packAt */
extern "C" Sprite *FUN_80063a5c(Sprite *p, int f16, char *name, int a3, int x, int mw, int mh) {
	unsigned short pal;
	char *data;
	int w;
	int h;
	SlicedImage *v;

	p->__vptr = __vt_6Sprite;
	Load8BitBMP2(name, &data, &w, &h, &pal);

	p->m_bpp = f16;
	p->m_w = w;
	p->m_h = h;

	if (!(mw * 2 < w))
		mw = 0;

	if (!(mh < h))
		mh = 0;

	v = new SlicedImage(data, w, h, mw, mh, p->m_bpp, pal);

	p->m_image = v;
	data = 0;
	p->m_packed = 1;
	FUN_800627ec(v, a3, x);
	return p;
}

/* _._6Sprite (0x80063b64) */
/* Deletes the sprite's sliced image. */
Sprite::~Sprite() {
	delete m_image;
}

/* draw__6Spriteiiiiii (0x80063be4) */
/* Draws a packed sprite's slices (tinted or flat, opaque) or, unpacked, via
   rotateDraw at (x, y). */
void Sprite::draw(int x, int y, int otFront, int r, int g, int b) {
	if (m_packed)
		m_image->drawScaled(x, y, m_w, m_h, otFront, r, g, b, 0, 0);
	else
		m_image->rotateDraw(x, y);
}

/* prepare__6Sprite (0x80063c48) */
/* Sprite (game/shellutils.h) */
void Sprite::prepare() {
	FUN_80063290(m_image);
}

/* drawFaded__6Spriteiiiiii (0x80063c6c) */
/* Draws the sprite semi-transparent. */
void Sprite::drawFaded(int x, int y, int otFront, int r, int g, int b) {
	m_image->drawScaled(x, y, m_w, m_h, otFront, r, g, b, 1, 0);
}

/* pack__6Sprite (0x80063cb4) */
/* Moves the image into VRAM (once). */
void Sprite::pack() {
	if (!m_packed) {
		m_packed = 1;
		m_image->pack();
	}
}

/* unpack__6Sprite (0x80063cec) */
/* Releases the image's VRAM if it was packed. */
void Sprite::unpack() {
	if (m_packed) {
		m_packed = 0;
		m_image->unpack();
	}
}

/* removeFromMemory__6Sprite (0x80063d24) */
/* Frees the image's pixel copy in main RAM. */
void Sprite::removeFromMemory() {
	m_image->removeFromMemory();
}

/* __11EraseSpriteiiii (0x80063d48) */
/* Saves the VRAM rectangle (x, y, w, h) to main RAM so it can be restored. */
EraseSprite::EraseSprite(int x, int y, int w, int h) {
	mR.x = x;
	mR.y = y;
	mR.w = w;
	mR.h = h;
	mData = (ulong *)Mem_NewX(w * h * 2, 1, 1);
	PC_ScreenRead(mR.x, mR.y, mR.w, mR.h, mData);
}

/* _._11EraseSprite (0x80063db4) */
/* Frees the saved pixels. */
EraseSprite::~EraseSprite() {
	Mem_Delete(mData);
}

/* restore__11EraseSpriteii (0x80063e10) */
/* Puts the saved pixels back where they came from (x, y unused). */
void EraseSprite::restore(int x, int y) {
	PC_ScreenBlit(mR.x, mR.y, mR.w, mR.h, mData);
}

/* __9PadBufferi (0x80063e34) */
/* An empty d-pad queue for controller pad (both when >= 2; forced to pad 0
   when DAT_800d1d9c is set); clears the d-pad pressed flags of the pad(s) it reads. */
PadBuffer::PadBuffer(int pad) {
	entries = 0;
	down = 0;
	last = 0;
	first = 0;
	mWhich_pad = pad;

	if (DAT_800d1d9c)
		mWhich_pad = 0;

	if (mWhich_pad < 2) {
		Pad[mWhich_pad].MenuLeft.Pressed = 0;
		Pad[mWhich_pad].MenuRight.Pressed = 0;
		Pad[mWhich_pad].MenuUp.Pressed = 0;
		Pad[mWhich_pad].MenuDown.Pressed = 0;
	} else {
		Pad[0].MenuLeft.Pressed = 0;
		Pad[0].MenuRight.Pressed = 0;
		Pad[0].MenuUp.Pressed = 0;
		Pad[0].MenuDown.Pressed = 0;
		Pad[1].MenuLeft.Pressed = 0;
		Pad[1].MenuRight.Pressed = 0;
		Pad[1].MenuUp.Pressed = 0;
		Pad[1].MenuDown.Pressed = 0;
	}
}

/* update__9PadBuffer (0x80063f58) */
/* PadBuffer: a small queue of d-pad presses for the front-end menus. update() samples the
   Left/Right/Up/Down "Pressed" flags of one controller (or of both when mWhich_pad >= 2), queues one
   press (1..4) per push and waits for every direction to be released before queueing again. */
void PadBuffer::update() {
	if (!down) {
		current = 0;

		if (mWhich_pad < 2) {
			if (Pad[mWhich_pad].MenuLeft.Pressed)
				current = 1;

			if (Pad[mWhich_pad].MenuRight.Pressed)
				current = 2;

			if (Pad[mWhich_pad].MenuUp.Pressed)
				current = 3;

			if (Pad[mWhich_pad].MenuDown.Pressed)
				current = 4;
		} else {
			if (Pad[0].MenuLeft.Pressed || Pad[1].MenuLeft.Pressed)
				current = 1;

			if (Pad[0].MenuRight.Pressed || Pad[1].MenuRight.Pressed)
				current = 2;

			if (Pad[0].MenuUp.Pressed || Pad[1].MenuUp.Pressed)
				current = 3;

			if (Pad[0].MenuDown.Pressed || Pad[1].MenuDown.Pressed)
				current = 4;
		}

		if (current && entries < 8) {
			buffer[last++] = current;

			if (last >= 8)
				last = 0;

			down = 1;
			entries++;
		}
	} else {
		if (mWhich_pad < 2) {
			if (!Pad[mWhich_pad].MenuLeft.Pressed && !Pad[mWhich_pad].MenuRight.Pressed &&
			    !Pad[mWhich_pad].MenuUp.Pressed && !Pad[mWhich_pad].MenuDown.Pressed)
				down = 0;
		} else {
			if (!Pad[0].MenuLeft.Pressed && !Pad[0].MenuRight.Pressed && !Pad[0].MenuUp.Pressed &&
			    !Pad[0].MenuDown.Pressed && !Pad[1].MenuLeft.Pressed && !Pad[1].MenuRight.Pressed &&
			    !Pad[1].MenuUp.Pressed && !Pad[1].MenuDown.Pressed)
				down = 0;
		}
	}
}

/* getPress__9PadBuffer (0x80064274) */
/* Pops the oldest queued d-pad press (1..4), or 0 if none. */
int PadBuffer::getPress() {
	int press = 0;

	if (entries > 0) {
		entries--;
		press = buffer[first];

		if (++first >= 8)
			first = 0;
	}

	return press;
}

/* reset__9PadBuffer (0x800642c0) */
/* Empties the queue. */
void PadBuffer::reset() {
	entries = 0;
	down = 0;
	last = 0;
	first = 0;
}

/* PadBuffer_SetNumPlayers__Fi (0x800642d4) */
void PadBuffer_SetNumPlayers(int a) {
	DAT_800d1d9c = a == 1;
}

/* __12VerticalMenuiiiPPciPiT6i (0x800642e8) */
/* Demo twin (0.998 by opcode); locals from its MAIN.SYM. Builds a vertical
   menu: measures each item, centres the block between top and bottom, places each item (custom
   x/y if given) with a highlight bar sprite, draws it, then selects the initial item. */
VerticalMenu::VerticalMenu(int top, int bottom, int spacing, char **items, int num, int *xlist,
                           int *ylist, int selection) {
	int tot_h = 0;
	int y;

	item_spacing = spacing;
	num_items = num;
	font_scale = Mess_GetFontScale();
	font_scale_big = font_scale * 320 / 256;

	{
		int i;

		for (i = 0; i < num_items; i++) {
			strcpy(list[i].string, items[i]);

			list[i].w = Mess_TextWidth(items[i]);
			list[i].h = Mess_TextHeight(items[i]);
			list[i].x = xlist ? xlist[i] : 320;
			list[i].baseline = Mess_TextHeightAboveBaseline(list[i].string);
			list[i].grayed = 0;
			tot_h += item_spacing + list[i].h;
		}
	}

	y = (top + bottom - tot_h) / 2;

	{
		int i;

		for (i = 0; i < num_items; i++) {
			list[i].y = y + (list[i].h >> 1);
			y += item_spacing + list[i].h;

			if (ylist)
				list[i].y = ylist[i];
			else
				list[i].y = list[i].y;

			if (xlist && xlist[i] && xlist[i] != 320)
				list[i].erase_sprite_x = list[i].x - 230;
			else
				list[i].erase_sprite_x = 45;

			list[i].erase_sprite =
			    new Sprite(list[i].erase_sprite_x,
				           list[i].y - (list[i].h >> 1) * font_scale_big / font_scale - 1, 550,
				           list[i].h * font_scale_big / font_scale + 4, 1, 32, 128);
			DrawSync(0);

			if (list[i].grayed)
				Mess_SetRGB(64, 64, 64, 0);
			else
				Mess_SetRGB(122, 122, 122, 0);

			Mess_DrawText(list[i].x, list[i].y - (list[i].h >> 1) + list[i].baseline,
			              list[i].string, 0, 0, 256, 0);
		}
	}

	mTri1 = 0;
	mTri2 = 0;
	mEraseTri1 = 0;
	mEraseTri2 = 0;
	selected = selection;
	select(selection);
}

/* setString__12VerticalMenuiPc (0x800646b0) */
/* Changes item i's text; reselects it (to redraw the triangles) if it's the
   selected item and the selection isn't animating. */
void VerticalMenu::setString(int i, char *pString) {
	strcpy(list[i].string, pString);

	if (tick < 61 && i == selected)
		select(i);
}

/* _._12VerticalMenu (0x80064720) */
/* Deletes the selection triangles (unless they're shared), their erase
   sprites and every item's highlight bar. */
VerticalMenu::~VerticalMenu() {
	if (!mDontDeleteSprites) {
		delete mTri1;
		delete mTri2;
	}

	delete mEraseTri1;
	delete mEraseTri2;

	for (int i = 0; i < num_items; i++)
		delete list[i].erase_sprite;
}

/* setTriangles__12VerticalMenuP6SpriteT1 (0x80064890) */
/* Uses caller-owned selection triangles (so the menu won't delete them),
   keeping any already set. */
void VerticalMenu::setTriangles(Sprite *tri1, Sprite *tri2) {
	mDontDeleteSprites = 1;

	if (!mTri1)
		mTri1 = tri1;

	if (!mTri2)
		mTri2 = tri2;
}

/* FUN_800648c0 (0x800648c0) */
/* VerticalMenuItem / VerticalMenu (game/shellutils.h) */
void FUN_800648c0(VerticalMenu *m, int i, int on) {
	int j;

	if (!on) {
		m->list[i].grayed = 0;
		return;
	}

	m->list[i].grayed = 1;

	if (m->selected == i) {
		for (j = i + 1; j < m->num_items; j++)
			if (!m->list[j].grayed) {
				m->VerticalMenu::select(j);
				return;
			}

		for (j = 0; j < i; j++)
			if (!m->list[j].grayed) {
				m->VerticalMenu::select(j);
				return;
			}
	}
}

/* draw__12VerticalMenu (0x80064990) */
/* Fuzzy demo twin (0.98); locals and layouts from its MAIN.SYM. Draws a
   vertical text menu: erases the old selection triangles (ticks 63/64), redraws every item (the
   selected one large and orange, greyed ones dark), rebuilds the triangles' erase sprites at
   tick 61 (the selected item's width is re-measured at tick 63), and draws the selection triangles while the tick counts down. */
void VerticalMenu::draw() {
	int tri_off;
	int y;

	Mess_SetTextJustify(0);
	Mess_SetFontScale(font_scale);

	if ((uint)(tick - 63) < 2) {
		tri_off = (list[old_selected].w >> 1) * font_scale_big / font_scale + 30;

		if (mEraseTri1)
			mEraseTri1->draw(list[old_selected].x - tri_off - 22, list[old_selected].y - 34, 0, -1,
			                 -1, -1);

		if (mEraseTri2)
			mEraseTri2->draw(list[old_selected].x + tri_off - 22, list[old_selected].y - 34, 0, -1,
			                 -1, -1);

		if (tick == 63)
			list[selected].w = Mess_TextWidth(list[selected].string);
	}

	{
		int i;

		for (i = 0; i < num_items; i++) {
			list[i].erase_sprite->draw(
			    list[i].erase_sprite_x,
			    list[i].y - (list[i].h >> 1) * font_scale_big / font_scale - 1, 0, -1, -1, -1);

			if (i == selected) {
				Mess_SetRGB(182, 132, 36, 0);
				Mess_SetFontScale(font_scale_big);

				y = list[i].y - ((list[i].h >> 1) - list[i].baseline) * font_scale_big / font_scale;
			} else {
				if (list[i].grayed)
					Mess_SetRGB(64, 64, 64, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				y = list[i].y - (list[i].h >> 1) + list[i].baseline;
			}

			Mess_DrawText(list[i].x, y, list[i].string, 0, 0, 256, 0);
			Mess_SetFontScale(font_scale);
		}
	}

	tri_off = (list[selected].w >> 1) * font_scale_big / font_scale + 30;

	if (tick == 61) {
		if (mEraseTri1)
			delete mEraseTri1;

		mEraseTri1 = new Sprite(
		    list[selected].x - ((list[selected].w >> 1) * font_scale_big / font_scale + 30) - 22,
		    list[selected].y - 34, 44, 68, 1, 0, 0);

		if (mEraseTri2)
			delete mEraseTri2;

		mEraseTri2 = new Sprite(
		    list[selected].x + ((list[selected].w >> 1) * font_scale_big / font_scale + 30) - 22,
		    list[selected].y - 34, 44, 68, 1, 0, 0);
	}

	if ((uint)(tick - 16) < 45) {
		if (mTri1)
			mTri1->draw(list[selected].x - tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);

		if (mTri2)
			mTri2->draw(list[selected].x + tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);
	} else if ((uint)(tick - 14) < 2) {
		mEraseTri1->draw(list[selected].x - tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);
		mEraseTri2->draw(list[selected].x + tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);
	}

	if (--tick <= 0)
		tick = 60;
}

/* erase__12VerticalMenu (0x80064f9c) */
void VerticalMenu::erase() {
	int tri_off;

	tri_off = ((list[selected].w >> 1) * font_scale_big) / font_scale + 30;

	if (tick < 61) {
		if (mEraseTri1)
			mEraseTri1->draw(list[selected].x - tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);

		if (mEraseTri2)
			mEraseTri2->draw(list[selected].x + tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);
	}

	for (int i = 0; i < num_items; i++) {
		list[i].erase_sprite->draw(list[i].erase_sprite_x,
		                           list[i].y - ((list[i].h >> 1) * font_scale_big) / font_scale - 1,
		                           0, -1, -1, -1);
	}
}

/* select__12VerticalMenui (0x80065120) */
/* Selects item i unless it's greyed out, restarting the selection animation
   (tick 62 if the triangles are still at rest, else 64). */
void VerticalMenu::select(int i) {
	if (!list[i].grayed) {
		old_selected = selected;
		selected = i;

		if (tick > 60)
			tick = 62;
		else
			tick = 64;
	}
}

/* getSelected__12VerticalMenu (0x8006516c) */
/* The selected item. */
int VerticalMenu::getSelected() {
	return selected;
}

/* up__12VerticalMenu (0x80065178) */
/* Selects the previous non-greyed item (wrapping); while the selection is
   still animating, swallows the button presses instead. */
void VerticalMenu::up() {
	if (tick > 60) {
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);
	} else {
		int i = selected - 1;

		if (i < 0)
			i = num_items - 1;

		while (i != selected && list[i].grayed) {
			i--;

			if (i < 0)
				i = num_items - 1;
		}

		select(i);
	}
}

/* down__12VerticalMenu (0x80065230) */
/* Selects the next non-greyed item (wrapping); while the selection is still
   animating, swallows the button presses instead. */
void VerticalMenu::down() {
	if (tick > 60) {
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);
	} else {
		int i = selected + 1;

		if (i >= num_items)
			i = 0;

		while (i != selected && list[i].grayed) {
			i++;

			if (i >= num_items)
				i = 0;
		}

		select(i);
	}
}

/* FUN_800652e4 (0x800652e4) */
char **FUN_800652e4(char **p, int a, int b, int c, char **d, int e, int *f, int *g, int h) {
	__12VerticalMenuiiiPPciPiT6i(p, a, b, c, d, e, f, g, h);

	*p = __vt_14VerticalDialog;
	return p;
}

/* _._14VerticalDialog (0x80065340) */
/* Nothing of its own; VerticalMenu's destructor does the work. */
VerticalDialog::~VerticalDialog() {}

/* draw__14VerticalDialog (0x80065394) */
/* A dialog draws every item plainly: its bar sprite, then its text in grey
   (dark grey if greyed out); no selection highlight or triangles. */
void VerticalDialog::draw() {
	Mess_SetTextJustify(0);
	Mess_SetFontScale(font_scale);

	for (int i = 0; i < num_items; i++) {
		list[i].erase_sprite->draw(list[i].erase_sprite_x,
		                           list[i].y - (list[i].h >> 1) * font_scale_big / font_scale - 1,
		                           0, -1, -1, -1);

		if (list[i].grayed)
			Mess_SetRGB(64, 64, 64, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText(list[i].x, list[i].y - (list[i].h >> 1) + list[i].baseline, list[i].string, 0,
		              0, 256, false);
		Mess_SetFontScale(font_scale);
	}
}

/* FUN_80065520 (0x80065520) */
FileLoader *FUN_80065520(FileLoader *p) {
	*(char **)p = DAT_800a2130;
	p->mNum = 0;
	p->mMode = 0;
	return p;
}

/* FUN_8006553c (0x8006553c) */
void FUN_8006553c(char **p, int f) {
	*p = DAT_800a2130;
	FUN_80065d80((FileLoader *)p);

	*p = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* setFiles__10FileLoaderPPci (0x80065594) */
/* C++ (SHELLUTILS.CPP, THPS1-only; no demo twin). A front-end file loader object (vtable
   0x800a2130, 32 entries of 36 bytes). This is its "load this set of files" call: it syncs
   any pending read, keeps entries that are still wanted and loaded (compacting them to the
   front), unloads the rest (FileLoader::unload), appends the new names as unloaded entries, and
   blanks the unused tail. FrontLoader_SetFiles calls it with a per-screen file list. */
void FileLoader::setFiles(char **names, int num) {
	int keep[32];
	int i;
	int j;
	int n;

	if (mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	for (int k = 0; k < 32; k++)
		keep[k] = 0;

	n = 0;

	for (i = 0; i < mNum; i++) {
		for (j = 0; j < num; j++)
			if (!strcmp(mEntry[i].Name, names[j]))
				break;

		if (j < num && mEntry[i].Loaded) {
			keep[j] = 1;
			mEntry[n++] = mEntry[i];
		} else
			unload(i);
	}

	for (int f = 0; f < num; f++) {
		if (!keep[f]) {
			strcpy(mEntry[n].Name, names[f]);

			mEntry[n].Loaded = 0;
			n++;
		}
	}

	mNum = n;

	for (int e = n; e < 32; e++) {
		strcpy(mEntry[e].Name, "URB");

		mEntry[e].Loaded = 0;
	}

	mCur = 0;
	mMode = 0;
	FileIO_SetSectorsPerFrame(4);
}

/* FUN_8006582c (0x8006582c) */
/* SLoadEntry / FileLoader (game/shellutils.h) */
extern "C" void FUN_8006582c(FileLoader *a) {
	char *s;
	int ok, n, t;
	char buf[16];

	if (a->mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	while (a->mEntry[a->mCur].Loaded)
		a->mCur++;

	a->mMode = 0;
	s = a->mEntry[a->mCur].Name;

	/* (`&`, not `&&`: the test as the C original's compiler folded it) */
	while ((*s != 95) & (*s != 0))
		s++;

	if (*s == 95) {
		s++;
		ok = 0;

		if (!strcmp(s, "fe") || !strcmp(s, "FE") || !strcmp(s, "f") || !strcmp(s, "F"))
			ok = 1;

		if (ok)
			a->mMode = 1;
	}

	s = a->mEntry[a->mCur].Name;

	while ((*s != 46) & (*s != 0))
		s++;

	if (*s == 46) {
		s++;
		ok = 0;

		if (!strcmp(s, "pre") || !strcmp(s, "PRE"))
			ok = 1;

		if (ok)
			a->mMode = 2;
	}

	t = a->mMode;

	switch (t) {
		case 1:
			a->mEntry[a->mCur].pData = 0;
			a->mEntry[a->mCur].Handle = Spool_PSX(a->mEntry[a->mCur].Name, 2, 1);
			a->mEntry[a->mCur].Size = 0;
			a->mEntry[a->mCur].Type = t;
			break;
		case 2:
			SpoolerBlocksFileIO->spoolPRE(a->mEntry[a->mCur].Name);

			a->mEntry[a->mCur].pData = 0;
			a->mEntry[a->mCur].Handle = -1;
			a->mEntry[a->mCur].Size = 0;
			a->mEntry[a->mCur].Type = t;
			break;
		default:
			n = FileIO_Open(a->mEntry[a->mCur].Name);

			if (n <= 0) {
				printf("\nFile %s not found", a->mEntry[a->mCur].Name);
				return;
			}

			a->mEntry[a->mCur].pData = Mem_NewX(n, 1, 1);

			if (a->mEntry[a->mCur].pData == 0) {
				printf("\nNot enough memory to load %s", a->mEntry[a->mCur].Name);
				return;
			}

			FileIO_Load(a->mEntry[a->mCur].pData);

			a->mEntry[a->mCur].Handle = -1;
			a->mEntry[a->mCur].Size = n;
			a->mEntry[a->mCur].Type = 0;
			break;
	}

	a->mEntry[a->mCur].Loaded = 1;
	a->mCur = a->mCur + 1;
}

/* FUN_80065ca4 (0x80065ca4) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
extern "C" void FUN_80065ca4(FileLoader *p) {
	FileIO_Continue();

	if (p->mMode == 1)
		Spool_Continue();

	if (p->mCur < p->mNum && Loading == 0)
		FUN_8006582c(p);
}

/* FUN_80065d14 (0x80065d14) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
extern "C" void FUN_80065d14(FileLoader *p) {
	while (p->mCur < p->mNum)
		FUN_8006582c(p);

	if (p->mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();
}

/* FUN_80065d80 (0x80065d80) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
extern "C" void FUN_80065d80(FileLoader *p) {
	int i;

	if (p->mMode == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	for (i = 0; i < p->mNum; i++)
		p->unload(i);

	p->mCur = 0;
}

/* unload__10FileLoaderi (0x80065e04) */
/* C++. Frees file i of the set: a heap file's buffer, a PRE archive, or a spooled PSX region. */
void FileLoader::unload(int i) {
	SLoadEntry *e = &mEntry[i];

	if (e->Loaded) {
		if (e->Type == 0) {
			Mem_Delete(e->pData);

			e->pData = 0;
		}

		if (e->Type == 2) {
			/* (with no PRE archive loaded retail looked through a null manager in low RAM) */
			if (SpoolerBlocksFileIO)
				SpoolerBlocksFileIO->unloadPRE(e->Name);
		} else if (e->Type == 1) {
			ProcessNewPSX(e->Handle);
			Spool_ClearPSX(e->Name);
			Spool_Sync();

			e->Handle = -1;
		}

		e->Loaded = 0;
	}
}

/* FUN_80065f1c (0x80065f1c) */
/* C++. Takes file `name` out of a FileLoader's set (FrontLoader_TakeFile): its buffer, which the
   caller now owns (the entry forgets it and counts as not loaded), or 0 if the set has no such
   file. */
extern "C" void *FUN_80065f1c(FileLoader *p, char *name) {
	int i;
	void *r = 0;

	for (i = 0; i < p->mNum; i++) {
		if (strcmp(p->mEntry[i].Name, name) == 0) {
			r = p->mEntry[i].pData;
			p->mEntry[i].pData = 0;
			p->mEntry[i].Loaded = 0;
			break;
		}
	}

	return r;
}

/* FUN_80065fd4 (0x80065fd4) */
/* C++ (FileLoader fields, not int offsets past the vptr: 64-bit). */
extern "C" int FUN_80065fd4(FileLoader *p) {
	return p->mCur == p->mNum;
}

/* FUN_80065fec (0x80065fec) */
/* SLoadEntry / FileLoader (game/shellutils.h) */
void FUN_80065fec(FileLoader *s) {
	void *tmp[32];
	int i;

	for (i = 0; i < s->mNum; i++) {
		if (s->mEntry[i].Loaded && !s->mEntry[i].Type) {
			tmp[i] = Mem_NewTop(s->mEntry[i].Size);
			Mem_Copy(tmp[i], s->mEntry[i].pData, s->mEntry[i].Size);
			Mem_Delete(s->mEntry[i].pData);
		}
	}

	for (i = 0; i < s->mNum; i++) {
		if (s->mEntry[i].Loaded && !s->mEntry[i].Type) {
			s->mEntry[i].pData = Mem_NewX(s->mEntry[i].Size, 1, 1);
			Mem_Copy(s->mEntry[i].pData, tmp[i], s->mEntry[i].Size);
			Mem_Delete(tmp[i]);
		}
	}
}

/* FUN_8006613c (0x8006613c) */
PREManager *FUN_8006613c(PREManager *p) {
	int i;

	*(char **)p = DAT_800a2120;

	for (i = 0; i < 16; i++) {
		p->mPre[i] = 0;
		strcpy(p->mPreName[i], "");
	}

	return p;
}

/* FUN_80066180 (0x80066180) */
void FUN_80066180(PREManager *p, int f) {
	int i;

	*(char **)p = DAT_800a2120;

	for (i = 0; i < 16; i++) {
		if (p->mPre[i])
			Mem_Delete(p->mPre[i]);
	}

	*(char **)p = DAT_800a21b0;

	if (f & 1)
		__dl__6CClassPv(p);
}

/* FUN_80066214 (0x80066214) */
void FUN_80066214(void *pm, char *name) {
	((PREManager *)pm)->spoolPRE(name);
	FileIO_Sync();
}
