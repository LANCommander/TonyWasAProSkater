/* Db_Render__Fv (0x80049094) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Render: waits for the other buffer's render, sets the draw area to
   this buffer's 512x240 window, links the clip prim into the OT, kicks off the draw and swaps the
   two buffer pointers. THPS1's SDoubleBuffer embeds the OT (0x84..0x8090) instead of pointing at it.
   FUN_8008cf34 = SetDrawArea; FUN_80089ddc (the OT draw) returns a value (call_value matters). */
struct RECT {
	short x, y, w, h;
};

struct DR_AREA {
	unsigned long tag;
	unsigned long code[2];
};

struct SDoubleBuffer {
	char Draw[0x5c];
	char Disp[0x14];
	int Visible;     /* 0x70 */
	int RenderNow;   /* 0x74 */
	int Rendering;   /* 0x78 */
	int FlipNow;     /* 0x7c */
	int LastVblanks; /* 0x80 */
	unsigned long OrderingTable[(0x8090 - 0x84) / 4];
	int Top;              /* 0x8090 */
	RECT Window;          /* 0x8094 */
	DR_AREA Clip;         /* 0x809c */
	unsigned char *Polys; /* 0x80a8 */
};

extern SDoubleBuffer DoubleBuffer[2];

SDoubleBuffer *pDoubleBuffer;
SDoubleBuffer *pOtherBuffer;

void Pause(int);
extern "C" void FUN_8008cf34(DR_AREA *, RECT *);
extern "C" int FUN_80048c74(void);
extern "C" int FUN_80089ddc(unsigned long *, int);

#define setRECT(r, _x, _y, _w, _h) (r)->x = (_x), (r)->y = (_y), (r)->w = (_w), (r)->h = (_h)
#define setaddr(p, _addr)                                                                          \
	(((*(unsigned long *)(p)) =                                                                    \
	      ((*(unsigned long *)(p)) & 0xff000000) | ((unsigned long)(_addr) & 0x00ffffff)))
#define getaddr(p)     (unsigned long)((*(unsigned long *)(p)) & 0x00ffffff)
#define addPrim(ot, p) setaddr(p, getaddr(ot)), setaddr(ot, p)

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif

void Db_Render() {
	if (pOtherBuffer->RenderNow) {
#ifdef PC_PORT
		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr, "db_render_stall f%u other=%p RenderNow=%d Rendering=%d\n", pc_heartbeat,
			        (void *)pOtherBuffer, pOtherBuffer->RenderNow, pOtherBuffer->Rendering);
		}
#endif
		Pause(1);
	}

	setRECT(&pDoubleBuffer->Window, 0, pDoubleBuffer->Top, 512, 240);
	FUN_8008cf34(&pDoubleBuffer->Clip, &pDoubleBuffer->Window);

	pDoubleBuffer->Clip.tag =
	    (pDoubleBuffer->Clip.tag & 0xff000000) | (pDoubleBuffer->OrderingTable[1] & 0xffffff);
	pDoubleBuffer->OrderingTable[1] = (pDoubleBuffer->OrderingTable[1] & 0xff000000) |
	                                 ((unsigned long)&pDoubleBuffer->Clip & 0xffffff);
	FUN_80089ddc(pDoubleBuffer->OrderingTable, FUN_80048c74());

	pDoubleBuffer->RenderNow = 1;
#ifdef PC_PORT
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "db_render f%u buf=%p\n", pc_heartbeat, (void *)pDoubleBuffer);
	}
#endif
	SDoubleBuffer *q = DoubleBuffer;
	SDoubleBuffer *p = q;

	if (pDoubleBuffer == q)
		p++;

	pDoubleBuffer = p;

	if (p == q)
		q++;

	pOtherBuffer = q;
}
