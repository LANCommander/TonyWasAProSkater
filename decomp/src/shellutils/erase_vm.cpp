/* erase__12VerticalMenu (0x80064f9c) */
#include "game/shellutils.h"

void VerticalMenu::erase() {
	int tri_off;

	tri_off = ((list[selected].w >> 1) * font_scale_big) / font_scale + 30;

	if (tick < 61) {
		if (mEraseTri1)
			mEraseTri1->draw(list[selected].x - tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);

		if (mEraseTri2)
			mEraseTri2->draw(list[selected].x + tri_off - 22, list[selected].y - 34, 0, -1, -1, -1);
	}

	for (int i = 0; i < num_items; i++) {
		list[i].erase_sprite->draw(list[i].erase_sprite_x,
		                           list[i].y - ((list[i].h >> 1) * font_scale_big) / font_scale - 1,
		                           0, -1, -1, -1);
	}
}
