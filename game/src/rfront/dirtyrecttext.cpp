/* Front_DirtyRectText__FiiPCc (0x80011324) */
#include "game/mess.h"
#include "game/rfront.h"
#include "game/shellutils.h"

extern SCachedBackground BGCache[10];
extern int FreeSprite;

int Mess_TextHeight(const char *); /* 0x8001ddb0 */

void Front_DirtyRectText(int x, int y, const char *s) {
	int i, w, h, nx;
	SCachedBackground *e;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].pString == s) {
			Mess_DrawText(x, y, s, 0, 0, 256, 0);
			return;
		}
	}

	BGCache[FreeSprite].pString = s;
	w = Mess_TextWidth(s);
	h = Mess_TextHeight(s);
	nx = x - (w >> 1);
	w += 4;
	e = &BGCache[FreeSprite];
	e->x = nx;
	e->y = y - h;
	h += 4;
	e->pSprite = new Sprite(nx, BGCache[FreeSprite].y, w, h, 1, 32, 128);
	BGCache[FreeSprite].State = 2;
	FreeSprite++;
}
