/* DownloadBMP__FPciiii (0x80024564) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/types.h"

void *Mem_New(unsigned int, int, int);
void Mem_Delete(void *);

void DownloadBMP(char *pData, int x, int y, int ClutX, int ClutY) {
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

	w = *(ushort *)(pData + 18);
	h = *(ushort *)(pData + 22);
	colors = *(ushort *)(pData + 46);

	if (colors == 0)
		colors = 256;

	pColor = pData + 54;
	transparent_color = 0;

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

	r.x = ClutX;
	r.y = ClutY;
	r.w = 256;
	r.h = 1;
	LoadImage(&r, (u_long *)colorTab);
	DrawSync(0);

	pad = w % 4;

	if (pad > 0)
		pad = 4 - pad;

	w += pad;
	data = (char *)Mem_New((w + 3) * h, 1, 1);
	in = pData + 54 + colors * 4;
	out = data + (h - 1) * w;

	{
		int j;
		int i;

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

	r.x = x;
	r.y = y;
	r.w = w / 2;
	r.h = h;
	LoadImage(&r, (u_long *)data);
	DrawSync(0);
	Mem_Delete(data);
}
