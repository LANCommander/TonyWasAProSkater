/* Video_Cleanup__Fv (0x80069670) */
#include "names.h"

extern int GVInfo1[];
extern int GVInfo2[];

void Video_Cleanup(void) {
	GVInfo1[0] = 0;
	GVInfo2[0] = 0;
}
