/* Front_FreeBackground__Fv (0x80011570) */
#include "game/rfront.h"
#include "game/shellutils.h"

extern int FreeSprite;
extern SCachedBackground BGCache[10];

void Front_FreeBackground() {
	int i;

	for (i = 0; i < FreeSprite; i++) {
		delete BGCache[i].pSprite;
		BGCache[i].State = 0;
	}

	FreeSprite = 0;
}
