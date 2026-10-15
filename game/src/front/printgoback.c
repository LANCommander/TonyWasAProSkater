/* Front_PrintGoBack__Fv (0x800178e4) */
#include "names.h"

typedef struct {
	signed char OffX, OffY;
	unsigned char Width, Height;
	void *pTexture;
} SAnimFrame;

extern SAnimFrame *ButtonsAnim;

extern const char *GameText[];

void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, const char *, int, int, int, int);
void *Panel_DrawTexturedPoly(SAnimFrame *);
void Panel_SetStretchedScreenCoords(int, int, void *, SAnimFrame *);

void Front_PrintGoBack(void) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(92, 214, GameText[101], 0, 0, 256, 0);
	Mess_SetTextJustify(0);

	p = Panel_DrawTexturedPoly(ButtonsAnim + 1);

	if (p)
		Panel_SetStretchedScreenCoords(64, 211, p, ButtonsAnim + 1);
}
