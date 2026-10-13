/* Db_Render__Fv (0x80049094) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Render: waits for the other buffer's render, sets the draw area to
   this buffer's 512x240 window, links the clip prim into the OT, kicks off the draw and swaps the
   two buffer pointers. THPS1's SDoubleBuffer embeds the OT (0x84..0x8090) instead of pointing at it.
   FUN_8008cf34 = SetDrawArea; FUN_80089ddc (the OT draw) returns a value (call_value matters). */
#include "game/m3d.h"

extern SDoubleBuffer DoubleBuffer[2];

SDoubleBuffer *pDoubleBuffer;
SDoubleBuffer *pOtherBuffer;

void Pause(int);
extern "C" void FUN_8008cf34(DR_AREA *, RECT *);
extern "C" int FUN_80048c74(void);
extern "C" int FUN_80089ddc(unsigned long *, int);

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif

void Db_Render() {
	if (pOtherBuffer->RenderNow) {
#ifdef PC_PORT
		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr, "db_render_stall f%u other=%p RenderNow=%d Rendering=%d\n",
			        pc_heartbeat, (void *)pOtherBuffer, pOtherBuffer->RenderNow,
			        pOtherBuffer->Rendering);
		}
#endif
		Pause(1);
	}

	setRECT(&pDoubleBuffer->Window, 0, pDoubleBuffer->Top, 512, 240);
	FUN_8008cf34(&pDoubleBuffer->Clip, &pDoubleBuffer->Window);

	pDoubleBuffer->Clip.tag =
	    (pDoubleBuffer->Clip.tag & 0xff000000) | (pDoubleBuffer->OT & 0xffffff);
	pDoubleBuffer->OT =
	    (pDoubleBuffer->OT & 0xff000000) | ((unsigned long)&pDoubleBuffer->Clip & 0xffffff);
	FUN_80089ddc(&pDoubleBuffer->OT2D, FUN_80048c74());

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
