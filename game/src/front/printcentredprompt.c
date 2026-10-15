/* Front_PrintCentredPrompt__FPCc (0x80017a30) */
#include "names.h"

typedef struct {
	signed char OffX, OffY;
	unsigned char Width, Height;
	void *pTexture;
} SAnimFrame;

SAnimFrame *ButtonsAnim;

void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, const char *, int, int, int, int);
void *Panel_DrawTexturedPoly(SAnimFrame *);
void Panel_SetStretchedScreenCoords(int, int, void *, SAnimFrame *);

void Front_PrintCentredPrompt(const char *s) {
	void *p;

	Mess_SetTextJustify(1);
	Mess_SetScale(192);
	Mess_SetRGB(150, 100, 25, 0);
	Mess_DrawText(227, 214, s, 0, 0, 256, 0);

	p = Panel_DrawTexturedPoly(ButtonsAnim);

	if (p)
		Panel_SetStretchedScreenCoords(192, 211, p, ButtonsAnim);
}
