/* Db_Render__Fv (0x80049094) flags=-fstructure-noalias */
/* C++ (DB.CPP). Demo twin Db_Render: waits for the other buffer's render, sets the draw area to
   this buffer's 512x240 window, links the clip prim into the OT, kicks off the draw and swaps the
   two buffer pointers. THPS1's SDoubleBuffer embeds the OT instead of pointing at it. */
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

#include <stdio.h>
#include <stdlib.h>

void Db_Render() {
	if (pOtherBuffer->RenderNow) {
		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr, "db_render_stall f%u other=%p RenderNow=%d Rendering=%d\n",
			        pc_heartbeat, (void *)pOtherBuffer, pOtherBuffer->RenderNow,
			        pOtherBuffer->Rendering);
		}
		Pause(1);
	}

	setRECT(&pDoubleBuffer->Window, 0, pDoubleBuffer->Top, 512, 240);
	SetDrawArea(&pDoubleBuffer->Clip, &pDoubleBuffer->Window);

	addPrim(&pDoubleBuffer->OT, &pDoubleBuffer->Clip);
	CompactOTagR(&pDoubleBuffer->OT2D, Db_OTSize());

	pDoubleBuffer->RenderNow = 1;
	if (getenv("THPS_TRACE_CULL")) {
		extern volatile unsigned pc_heartbeat;
		fprintf(stderr, "db_render f%u buf=%p\n", pc_heartbeat, (void *)pDoubleBuffer);
	}
	SDoubleBuffer *q = DoubleBuffer;
	SDoubleBuffer *p = q;

	if (pDoubleBuffer == q)
		p++;

	pDoubleBuffer = p;

	if (p == q)
		q++;

	pOtherBuffer = q;
}
