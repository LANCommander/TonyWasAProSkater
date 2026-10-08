/* Mess_TextHeight__FPCc (0x8001ddb0) */
#include "names.h"
extern int Mess_TextHeight2(const char *);
extern int Mess_TextHeightAboveBaseline(const char *);
extern int Mess_TextHeightBelowBaseline(const char *);

char NewFont;

int Mess_TextHeight(const char *s) {
	if (NewFont == 0)
		return Mess_TextHeight2(s);

	return Mess_TextHeightAboveBaseline(s) + Mess_TextHeightBelowBaseline(s);
}
