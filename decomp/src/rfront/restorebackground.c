/* Front_RestoreBackground__Fv (0x800114a8) */
#include "names.h"

typedef struct {
	void *spr;
	int pad;
	short x, y;
	int pad2;
	int type;
} SBg;

int FreeSprite;

extern SBg BGCache[];

void Sprite_draw(void *, int, int, int, int, int, int);

void Front_RestoreBackground(void) {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].type == 2)
			Sprite_draw(BGCache[i].spr, BGCache[i].x, BGCache[i].y, 0, -1, -1, -1);
	}
}
