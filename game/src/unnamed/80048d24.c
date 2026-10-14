/* Db_DrawPending__Fv (0x80048d24) */
#include "names.h"
#include "game/dbuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include "pc_draw.h"

extern int Db_OTSize(void);

/* Draws a buffer whose frame is ready once neither buffer is being drawn: its drawing
   environment, then its ordering table from the last entry. */
void Db_DrawPending(void) {
	int i;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].RenderNow) {
			if (!DoubleBuffer[i].Rendering && !DoubleBuffer[1 - i].Rendering &&
			    !DoubleBuffer[i].Visible) {
				DoubleBuffer[i].RenderNow = 0;
				DoubleBuffer[i].Rendering = 1;
				PC_DrawSetEnv(&DoubleBuffer[i].Draw);
				PC_DrawOT(&(&DoubleBuffer[i].OT2D)[Db_OTSize() - 1]);
				if (getenv("THPS_TRACE_CULL")) {
					extern volatile unsigned pc_heartbeat;
					fprintf(stderr, "d24_fired f%u buf=%d\n", pc_heartbeat, i);
				}
			}
			else if (getenv("THPS_TRACE_CULL")) {
				extern volatile unsigned pc_heartbeat;
				fprintf(stderr, "d24_blocked f%u buf=%d Rendering=%d,%d Visible=%d\n", pc_heartbeat,
				        i, DoubleBuffer[i].Rendering, DoubleBuffer[1 - i].Rendering,
				        DoubleBuffer[i].Visible);
			}
		}
	}
}
