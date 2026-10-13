/* Front_PrintSelect__Fv (0x80017978) */
#include "names.h"

typedef struct {
	int pad[2];
} SAnimFrame;

SAnimFrame *ButtonsAnim;

extern const char *GameText_198[];

void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, const char *, int, int, int, int);
void *Panel_DrawTexturedPoly(SAnimFrame *);
void Panel_SetStretchedScreenCoords(int, int, void *, SAnimFrame *);

void Front_PrintSelect(void) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(348, 214, GameText_198[0], 0, 0, 256, 0);
	Mess_SetTextJustify(0);

	p = Panel_DrawTexturedPoly(ButtonsAnim);

	if (p)
		Panel_SetStretchedScreenCoords(320, 211, p, ButtonsAnim);
}
