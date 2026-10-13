/* Front_DirtyRectText__FiiPCc (0x80011324) */
#include "names.h"

typedef struct {
	char *ptr;
	const char *str;
	short x, y;
	int pad;
	int state;
} T;

extern T BGCache[];

int FreeSprite;

void Mess_DrawText(int, int, const char *, int, int, int, int);
int Mess_TextWidth(const char *);
int Mess_TextHeight(const char *);
char *__nw__6CClassUi(unsigned);
char *__6Spriteiiiiiii(char *, int, int, int, int, int, int, int);

void Front_DirtyRectText(int x, int y, const char *s) {
	int i, w, h, nx;
	T *e;
	char *p;

	for (i = 0; i < FreeSprite; i++) {
		if (BGCache[i].str == s) {
			Mess_DrawText(x, y, s, 0, 0, 256, 0);
			return;
		}
	}

	BGCache[FreeSprite].str = s;
	w = Mess_TextWidth(s);
	h = Mess_TextHeight(s);
	nx = x - (w >> 1);
	w += 4;
	e = &BGCache[FreeSprite];
	e->x = nx;
	e->y = y - h;
	h += 4;
	p = __nw__6CClassUi(24);

	if (p)
		p = __6Spriteiiiiiii(p, nx, BGCache[FreeSprite].y, w, h, 1, 32, 128);

	e->ptr = p;
	BGCache[FreeSprite].state = 2;
	FreeSprite++;
}
