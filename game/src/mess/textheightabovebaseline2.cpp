/* Mess_TextHeightAboveBaseline2__FPCc (0x8001d528) */
/* C++ (MESS.CPP). The old-font version of Mess_TextHeightAboveBaseline (which calls it when
   !NewFont): a tail call to Mess_TextHeight2, as in the demo (MESS.CPP order Mess_TextHeight2,
   Mess_TextHeightAboveBaseline2, Mess_TextHeightBelowBaseline2 = retail 0x8001d440/d528/d548).
   Was mislabelled CBruce::Trick_HoldTrick, which is 0x8002f438. */
#include "game/mess.h"

int Mess_TextHeight2(const char *);

int Mess_TextHeightAboveBaseline2(const char *s) {
	return Mess_TextHeight2(s);
}
