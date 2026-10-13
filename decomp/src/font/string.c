/* Font_String__FUsUsPcUcUcUcUc (0x8004c1a8) */
#include "names.h"

typedef struct {
	int a;
	short x, y;
	char *s;
	int col;
	unsigned char z, w;
	char pad[14];
} SFont;

SFont *BufferPos;
int CodeVal;

void Font_String(unsigned short x, unsigned short y, char *s, unsigned char c0, unsigned char c1,
                 unsigned char c2, unsigned char c3) {
	SFont *p = BufferPos;

	p->z = 0;
	p->a = 0;
	p->x = x;
	p->y = y;
	BufferPos->w = c3;
	BufferPos->col = CodeVal | (c2 << 16) | (c1 << 8) | c0;
	BufferPos->s = s;
	BufferPos++;
	BufferPos->a = 1;
}
