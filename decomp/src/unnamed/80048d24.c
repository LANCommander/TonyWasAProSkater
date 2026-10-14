/* Db_DrawPending__Fv (0x80048d24) */
#include "names.h"

typedef struct {
	char p0[0x70];
	int f70;
	int f74;
	int f78;
	int f7c;
	int f80;
	int arr[(0x80ac - 0x84) / 4];
} E;

extern E DoubleBuffer[];

extern int Db_OTSize(void);
extern int FUN_8008c94c(void *a);
extern int DrawOTag(void *a);

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#include "pc_draw.h"
#endif

void Db_DrawPending(void) {
	int i;
	int *q;

	for (i = 0; i < 2; i++) {
		if (DoubleBuffer[i].f74) {
			if (!DoubleBuffer[i].f78 && !DoubleBuffer[1 - i].f78 && !DoubleBuffer[i].f70) {
				DoubleBuffer[i].f74 = 0;
				DoubleBuffer[i].f78 = 1;
#ifdef PC_PORT
				PC_DrawSetEnv((void *)(&DoubleBuffer[i]));
#else
				FUN_8008c94c(&DoubleBuffer[i]);
#endif

				q = &DoubleBuffer[i].arr[Db_OTSize()];
#ifdef PC_PORT
				PC_DrawOT((void *)(q - 1));
#else
				DrawOTag(q - 1);
#endif
#ifdef PC_PORT
				if (getenv("THPS_TRACE_CULL")) {
					extern volatile unsigned pc_heartbeat;
					fprintf(stderr, "d24_fired f%u buf=%d\n", pc_heartbeat, i);
				}
#endif
			}
#ifdef PC_PORT
			else if (getenv("THPS_TRACE_CULL")) {
				extern volatile unsigned pc_heartbeat;
				fprintf(stderr, "d24_blocked f%u buf=%d Rendering=%d,%d Visible=%d\n", pc_heartbeat,
				        i, DoubleBuffer[i].f78, DoubleBuffer[1 - i].f78, DoubleBuffer[i].f70);
			}
#endif
		}
	}
}
