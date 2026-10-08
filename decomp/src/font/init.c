/* Font_Init__Fv (0x8004c14c) */
#include "names.h"
extern char s_Fontsmll[];
extern char s_Font[];

int FontAnim, BigFontAnim;

int Spool_FindAnim(char *);

void Font_Init(void) {
	FontAnim = Spool_FindAnim(s_Fontsmll);
	BigFontAnim = Spool_FindAnim(s_Font);
}
