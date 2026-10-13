/* Db_Render__Fv (0x80049094) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Render: waits for the other buffer's render, sets the draw area to
   this buffer's 512x240 window, links the clip prim into the OT, kicks off the draw and swaps the
   two buffer pointers. THPS1's SDoubleBuffer embeds the OT (0x84..0x8090) instead of pointing at it.
   SetDrawArea = SetDrawArea; CompactOTagR (the OT draw) returns a value (call_value matters). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/db.h"
#include "game/m3d.h"
#include "game/m3dasm.h"

extern SDoubleBuffer DoubleBuffer[2];

SDoubleBuffer *pDoubleBuffer;
SDoubleBuffer *pOtherBuffer;

void Pause(int);

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
	SetDrawArea(&pDoubleBuffer->Clip, &pDoubleBuffer->Window);

	pDoubleBuffer->Clip.tag =
	    (pDoubleBuffer->Clip.tag & 0xff000000) | (pDoubleBuffer->OT & 0xffffff);
	pDoubleBuffer->OT =
	    (pDoubleBuffer->OT & 0xff000000) | ((unsigned long)&pDoubleBuffer->Clip & 0xffffff);
	CompactOTagR(&pDoubleBuffer->OT2D, Db_OTSize());

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
