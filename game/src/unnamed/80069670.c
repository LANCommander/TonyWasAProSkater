/* Video_Cleanup__Fv (0x80069670) */
#include "game/classsizes.h"
#include "names.h"

extern int GVInfo1[]; /* SVideoInfo[2] */

void Video_Cleanup(void) {
	GVInfo1[0] = 0;
	GVInfo1[SIZEOF_SVIDEOINFO / 4] = 0; /* GVInfo1[1].VideoMode */
}
