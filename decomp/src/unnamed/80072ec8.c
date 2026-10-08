/* DisplayTitle__5CTape (0x80072ec8) */
#include "names.h"
int CTape_mOkayToDisplayTitle;

int CTape_DisplayTitle(void) {
	return CTape_mOkayToDisplayTitle != 0;
}
