/* Front_PrintDemo__Fv (0x800160bc) */
#include "names.h"

int DAT_800d13ec;

extern int GNumberOfPlayers;

void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, const char *, int, int, int, int);

void Front_PrintDemo(void) {
	if (--DAT_800d13ec == 0)
		DAT_800d13ec = 48;

	if (DAT_800d13ec >= 13) {
		if (GNumberOfPlayers == 2) {
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(128, 35, "demo mode", 1, 0, 256, 0);
			Mess_DrawText(384, 35, "demo mode", 1, 0, 256, 0);
		} else {
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
			Mess_DrawText(256, 35, "demo mode", 1, 0, 256, 0);
		}
	}
}
