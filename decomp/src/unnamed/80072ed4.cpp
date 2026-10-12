/* DisplayContinueButton__5CTape (0x80072ed4) */
/* C++ (TAPES.CPP). Whether the tape message's continue button may be shown. */
#include "game/tapes.h"

int CTape::DisplayContinueButton() {
	return mOkayToDisplayContinueButton != 0;
}
