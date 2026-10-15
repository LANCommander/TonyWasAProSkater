/* Front_RestoreBackground__Fv (0x800114a8) */
#include "game/rfront.h"
#include "game/shellutils.h"

extern int FreeSprite;
extern SCachedBackground BGCache[10];

void Front_RestoreBackground() {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].State == 2)
			BGCache[i].pSprite->draw(BGCache[i].x, BGCache[i].y, 0, -1, -1, -1);
	}
}
