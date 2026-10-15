/* Mess_GetScale__Fv (0x8001d290) */
#include "names.h"

extern unsigned char NewFont;
extern unsigned short Scale;

extern int Mess_GetFontScale(void);

int Mess_GetScale(void) {
	if (NewFont)
		return Mess_GetFontScale();

	return Scale;
}
