/* Competition_MarqueeDisplay__Fv (0x800764d8) */
#include "names.h"

typedef struct {
	char pad[0xd4];
	char *text;
	char pad2[0x124 - 0xd8];
	unsigned x;
} SComp;

extern SComp theCompetition;

void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetScale(int);
void Mess_SetTextJustify(int);
int Mess_DrawText(int, int, const char *, int, int, int, int);

void Competition_MarqueeDisplay(void) {
	if (theCompetition.text) {
		Mess_SetRGB(122, 122, 122, 0);
		Mess_SetScale(256);
		Mess_SetTextJustify(1);

		if (theCompetition.x > 768)
			theCompetition.x = 0;

		Mess_DrawText(512 - theCompetition.x, 200, theCompetition.text, 0, 0, 256, 0);

		theCompetition.x += 5;
	}
}
