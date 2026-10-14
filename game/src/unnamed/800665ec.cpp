/* MakeGreyClut__Fii (0x800665ec) */
/* C++ (IMAGE.CPP). Sibling of the demo's MakeTransparentClut2 (same locals): reads the 256-entry
   CLUT at (x, y) back from VRAM, turns every colour into an opaque grey of 55% of r+g+b, and
   uploads it again. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/mem.h"
#include "pc_texstore.h"

void MakeGreyClut(int x, int y) {
	unsigned short *buf = (unsigned short *)Mem_NewX(512, 1, 1);
	RECT temp;
	unsigned short *buf2;

	temp.x = x;
	temp.y = y;
	temp.w = 256;
	temp.h = 1;

	PC_TexRead(temp.x, temp.y, temp.w, temp.h, buf);

	buf2 = buf;

	for (int i = 0; i < 256; i++) {
		unsigned int r = (*buf2 >> 10) & 31;
		unsigned int g = (*buf2 >> 5) & 31;
		unsigned int b = *buf2 & 31;
		unsigned int grey = (r + g + b) * 550 / 3000;

		*buf2 = ((grey << 10) + (grey << 5) + grey) | 0x8000;
		buf2++;
	}

	PC_TexUpload(temp.x, temp.y, temp.w, temp.h, buf);
	Mem_Delete(buf);
}
