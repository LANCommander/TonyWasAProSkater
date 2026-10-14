/* DisplayTitle__5CTape (0x80072ec8) */
/* C++ (TAPES.CPP). Whether tape titles may be shown. */
#include "game/tapes.h"

int CTape::DisplayTitle() {
	return mOkayToDisplayTitle != 0;
}
