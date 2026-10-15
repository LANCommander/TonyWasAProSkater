/* The display double buffer (DB.CPP): DoubleBuffer[2] and pDoubleBuffer, the one being built. C and
   C++ (game/m3d.h includes it). */
#ifndef GAME_DBUFFER_H
#define GAME_DBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#include <libgpu.h>
#ifdef __cplusplus
}
#endif

/* one display buffer. THPS1 embeds the ordering tables, which the demo pointed at
   (PanelOT/OrderingTable/ViewportOT). Its entries are drawn from OT3D's last down to OT2D: OT2D,
   OT and OT3D are one table (Db_OTSize entries from OT2D). Retail offsets; the port's entries are
   native tags (libgpu.h P_TAG). */
typedef struct SDoubleBuffer {
	DRAWENV Draw;          /* 0x0000 */
	DISPENV Disp;          /* 0x005c */
	int Visible;           /* 0x0070 */
	int RenderNow;         /* 0x0074 */
	int Rendering;         /* 0x0078 */
	int FlipNow;           /* 0x007c */
	int LastVblanks;       /* 0x0080 */
	P_TAG OT2D;            /* 0x0084: the 2D/panel OT entry */
	P_TAG OT;              /* 0x0088 */
	P_TAG OT3D[0x2000];    /* 0x008c: two players' 4096-entry layers */
	P_TAG *pOT;            /* 0x808c */
	int Top;               /* 0x8090 */
	RECT Window;           /* 0x8094 */
	DR_AREA Clip;          /* 0x809c */
	unsigned char *Polys;  /* 0x80a8 */
} SDoubleBuffer;

/* A buffer's packet memory (Polys) for what took `bytes` on the PS1: the port's packets have native
   tags (libgpu.h P_TAG), a word or more bigger than the PS1's (three in a 64-bit build). */
#define DB_PACKET_MEMORY(bytes) ((bytes) * (sizeof(void *) == 8 ? 4 : 2))

#ifdef __cplusplus
extern "C" {
#endif
extern SDoubleBuffer DoubleBuffer[2];
extern SDoubleBuffer *pDoubleBuffer;
#ifdef __cplusplus
}
#endif

#endif
