/* Competition_IsOver__Fv (0x8007409c) */
#include "names.h"

extern unsigned char theCompetition_c0[];

int Competition_IsOver(void) {
	return theCompetition_c0[0] >= 3;
}
