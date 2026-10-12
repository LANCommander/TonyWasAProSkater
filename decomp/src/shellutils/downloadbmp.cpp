/* FUN_80061a10 (0x80061a10) -- 8-bit BMP to 16bpp at VRAM (0,0); variant of DownloadBMP__FPciiii */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>

	int LoadImage(RECT *, u_long *);
	int DrawSync(int);
	void printf(char *, ...);
}

#include "game/types.h"

void *Mem_New(unsigned int, int, int);
void Mem_Delete(void *);

extern void *pBMR;

#ifdef PC_PORT
#include "pc_assets.h"
#include "pc_gpu.h"
#endif

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
#ifdef PC_PORT
	/* a Dreamcast image bigger than the screen it fills (pc_assets.h): kept whole */
	int lw = 0;
	int lh = 0;
	int hires = PC_BmpLayoutSize(pData, &lw, &lh) &&
	            (lw != *(ushort *)(pData + 18) || lh != *(ushort *)(pData + 22));
#endif

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
#ifdef PC_PORT
	if (hires)
		PC_GpuLoadImageHiRes(0, 0, lw, lh, (ushort *)data, w, h);
	else
#endif
	LoadImage(&r, (u_long *)data);
	DrawSync(0);
	Mem_Delete(data);
}
