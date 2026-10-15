/* PreProcessAnimPacket__FPUiT0 (0x8006ff48) */
/* C++. Registers a PSX file's anim packet (AnimPackets; Spool_FindAnim looks anims up by name) and
   marks the textures its frames use. A frame in the file is 8 bytes: offsets and size, then an
   index in the file's texture table. Retail put the Texture pointers into the frames; the port
   makes the packet's frames SAnimFrames of its own (AnimPacket::pFrames): each anim's after a
   slot whose last word is its frame count, as in the file (CBit::SetAnim, M3d_RenderSuperItem
   read it before the first frame). */
#include "game/m3d.h"
#include "game/mem.h"
#include "game/spool.h"

extern AnimPacket *AnimPackets;

/* the frame's texture, as the code here sees it (its first 16 bytes are the GPU words) */
struct STex {
	char pad[4];
	uchar f4;
	char pad5;
	ushort flags;
	char pad8;
	uchar f9, fa, fb;
};

void PreProcessAnimPacket(Texture **ppTextures, uint *data) {
	AnimPacket *node = (AnimPacket *)Mem_NewX(sizeof(AnimPacket), 0, 1);
	uint n, i, total = 0;
	uint *p;
	SAnimFrame *pFrame;

	node->pNext = AnimPackets;
	node->pPacket = data;
	AnimPackets = node;
	n = *data++;

	/* the frames, all anims' */
	for (i = 0, p = data; i < n; i++) {
		uint m = p[2];

		total += m;
		p += 3 + 2 * m;
	}

	pFrame = node->pFrames = (SAnimFrame *)Mem_NewX((total + n + 1) * sizeof(SAnimFrame), 0, 1);

	for (i = 0; i < n; i++) {
		uint m, j;

		data += 2;
		m = *data++;
		pFrame++;
		((uint *)pFrame)[-1] = m;

		for (j = 0; j < m; j++, pFrame++) {
			STex *t = (STex *)ppTextures[data[1]];

			*(uint *)pFrame = data[0]; /* OffX, OffY, Width, Height */
			pFrame->pTexture = (Texture *)t;
			t->flags |= 0x20;
			t->f4++;
			t->f9++;
			t->fa++;
			t->fb++;
			data += 2;
		}
	}
}
