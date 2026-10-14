/* DownloadBMP16x2__FPc (0x80061714) -- 8-bit BMP to 16bpp at VRAM (0,0), line-doubled; variant of DownloadBMP__FPciiii */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/psyq.h"
#include "game/types.h"

void *Mem_New(unsigned int, int, int);
void Mem_Delete(void *);

extern void *pBMR;

extern "C" void FUN_80061a10(char *); /* DownloadBMP16 */

#include "pc_assets.h"
#include "pc_frame.h"
#include "pc_screen.h"

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
