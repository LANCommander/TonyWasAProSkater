/* Font_Init__Fv (0x8004c14c) */
#include "names.h"

void *FontAnim, *BigFontAnim;

void *Spool_FindAnim(char *);

void Font_Init(void) {
	FontAnim = Spool_FindAnim("Fontsmll");
	BigFontAnim = Spool_FindAnim("Font");
}
