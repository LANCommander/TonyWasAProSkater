/* draw__14VerticalDialog (0x80065394) */
/* C++ (SHELLUTILS.CPP). A dialog draws every item plainly: its bar sprite, then its text in grey
   (dark grey if greyed out); no selection highlight or triangles. */
#include "game/shellutils.h"

void Mess_SetTextJustify(unsigned char);
void Mess_SetFontScale(unsigned int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, const char *, int, unsigned int, int, bool);

void VerticalDialog::draw() {
	Mess_SetTextJustify(0);
	Mess_SetFontScale(font_scale);

	for (int i = 0; i < num_items; i++) {
		list[i].erase_sprite->draw(list[i].erase_sprite_x,
		                           list[i].y - (list[i].h >> 1) * font_scale_big / font_scale - 1,
		                           0, -1, -1, -1);

		if (list[i].grayed)
			Mess_SetRGB(64, 64, 64, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText(list[i].x, list[i].y - (list[i].h >> 1) + list[i].baseline, list[i].string, 0,
		              0, 256, false);
		Mess_SetFontScale(font_scale);
	}
}
