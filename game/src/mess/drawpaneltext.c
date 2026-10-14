/* Mess_DrawPanelText__FiiPCciUii (0x8001d30c) */
#include "names.h"

extern void Mess_DrawText(int, int, const char *, int, unsigned, int, int);

void Mess_DrawPanelText(int x, int y, const char *s, int a, unsigned b, int c) {
	Mess_DrawText(x, y, s, a, b, c, 1);
}
