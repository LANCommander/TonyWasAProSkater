/* DisplayContinueButton__5CTape (0x80072ed4) */
#include "names.h"
int CTape_mOkayToDisplayContinueButton;

int CTape_DisplayContinueButton(void *this_) {
	return CTape_mOkayToDisplayContinueButton != 0;
}
