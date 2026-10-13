/* Panel_GetViewportMessOffset__FiPsT1 (0x8006d67c) */
#include "names.h"

void Panel_GetViewportMessOffset(int m, short *x, short *y) {
	switch (m) {
		case 0:
		case 1:
			*x = 0;
			*y = 0;
			break;
		case 4:
			*x = 0;
			*y = 60;
			break;
		case 6:
			*x = 128;
			*y = 60;
			break;
		case 2:
			*x = 128;
			*y = 0;
			break;
		case 5:
			*x = 0;
			*y = -60;
			break;
		case 7:
			*x = -128;
			*y = -60;
			break;
		case 3:
			*x = -128;
			*y = 0;
			break;
	}
}
