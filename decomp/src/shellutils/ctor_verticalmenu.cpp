/* __12VerticalMenuiiiPPciPiT6i (0x800642e8) */
/* C++ (SHELLUTILS.CPP). Demo twin (0.998 by opcode); locals from its MAIN.SYM. Builds a vertical
   menu: measures each item, centres the block between top and bottom, places each item (custom
   x/y if given) with a highlight bar sprite, draws it, then selects the initial item. */
#include "game/shellutils.h"

int Mess_GetFontScale();
int Mess_TextHeight(const char *);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);

extern "C" {
	void FUN_8008e904(char *, char *);
	int Mess_TextWidth__FPCc(char *);
	int Mess_TextHeightAboveBaseline__FPCc(char *);
	int FUN_8008c468(int);
	int Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
}

VerticalMenu::VerticalMenu(int top, int bottom, int spacing, char **items, int num, int *xlist,
                           int *ylist, int selection) {
	int tot_h = 0;
	int y;

	item_spacing = spacing;
	num_items = num;
	font_scale = Mess_GetFontScale();
	font_scale_big = font_scale * 320 / 256;

	{
		int i;

		for (i = 0; i < num_items; i++) {
			FUN_8008e904(list[i].string, items[i]);

			list[i].w = Mess_TextWidth__FPCc(items[i]);
			list[i].h = Mess_TextHeight(items[i]);
			list[i].x = xlist ? xlist[i] : 320;
			list[i].baseline = Mess_TextHeightAboveBaseline__FPCc(list[i].string);
			list[i].grayed = 0;
			tot_h += item_spacing + list[i].h;
		}
	}

	y = (top + bottom - tot_h) / 2;

	{
		int i;

		for (i = 0; i < num_items; i++) {
			list[i].y = y + (list[i].h >> 1);
			y += item_spacing + list[i].h;

			if (ylist)
				list[i].y = ylist[i];
			else
				list[i].y = list[i].y;

			if (xlist[i] && xlist[i] != 320)
				list[i].erase_sprite_x = list[i].x - 230;
			else
				list[i].erase_sprite_x = 45;

			list[i].erase_sprite = new Sprite(
			    list[i].erase_sprite_x, list[i].y - (list[i].h >> 1) * font_scale_big / font_scale - 1, 550,
			    list[i].h * font_scale_big / font_scale + 4, 1, 32, 128);
			FUN_8008c468(0);

			if (list[i].grayed)
				Mess_SetRGB(64, 64, 64, 0);
			else
				Mess_SetRGB(122, 122, 122, 0);

			Mess_DrawText__FiiPCciUiib(list[i].x, list[i].y - (list[i].h >> 1) + list[i].baseline, list[i].string, 0, 0,
			             256, 0);
		}
	}

	mTri1 = 0;
	mTri2 = 0;
	mEraseTri1 = 0;
	mEraseTri2 = 0;
	selected = selection;
	select(selection);
}
