/* Panel_ReplayInstructions__Fv (0x8006d858) */
#include "names.h"

extern int GVInfo1[];
extern int DemoTimer;
extern int Status;
extern const char *GameText[];

void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetScale(int);
void Mess_SetTextJustify(int);
int Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

void Panel_ReplayInstructions(void) {
	if (GVInfo1[0] == 2 && DemoTimer && Status != 20 && Status != 22 && Status != 21) {
		int m = GVInfo1[19];

		if ((m == 0 && (unsigned)(GVInfo1[7] - GVInfo1[8]) < 90) || m == 1) {
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetScale(188);
			Mess_SetTextJustify(0);
			Mess_DrawPanelText(256, 220, GameText[94], 0, 0, 256);
		}
	}
}
