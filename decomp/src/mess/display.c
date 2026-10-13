/* Mess_Display__Fv (0x8001d764) */
#include "names.h"

typedef struct {
	unsigned short us0;
	unsigned char b2, b3;
	unsigned short us4, us6;
	unsigned char b8, b9, b10, b11, b12, b13;
	unsigned short us14, us16;
	short s18, s20, s22, s24;
	unsigned char b26, _27;
	unsigned short us28;
} Anim;

typedef struct Node {
	char *text;
	unsigned short t;
	short _6;
	short s8, s10;
	int f12;
	Anim *anim;
	struct Node *next;
} Node;

Node *pMessages;
char TextJustification;
int FlagsBGR;
unsigned short Scale;

extern volatile unsigned Xblanks;

void Mess_SetScale(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

void Mess_Display(void) {
	Node *head;
	Node *n;
	Anim *a;
	int cnt, off, x, y, w, v, d;
	unsigned char cr, cg, cb;
	int sc, q4;
	unsigned short Dur;

	head = n = pMessages;
	cnt = 0;

	for (; n; n = n->next)
		if (n->f12)
			cnt++;

	off = (cnt - 1) << 4;

	for (n = head; n; n = n->next) {
		a = n->anim;

		if (a->b26 && !((n->t / a->b26) & 1))
			continue;

		Dur = a->us0;
		TextJustification = a->b3;
		Scale = ((a->us6 - a->us4) * n->t) / Dur + a->us4;
		Mess_SetScale(Scale);

		cr = a->b8 + ((a->b11 - a->b8) * n->t) / Dur;
		cg = a->b9 + ((a->b12 - a->b9) * n->t) / Dur;
		cb = a->b10 + ((a->b13 - a->b10) * n->t) / Dur;
		v = (cb << 16) | (cg << 8) | cr;
		FlagsBGR = v;

		if (a->b2)
			FlagsBGR |= 0x2e000000;
		else
			FlagsBGR |= 0x2c000000;

		if (a->us28)
			Mess_SetRGB(((Xblanks % 20) * 12) & 252, 100, 100, 0);

		x = a->s20 + ((a->s24 - a->s20) * n->t) / Dur + n->s10;
		y = a->s18 + ((a->s22 - a->s18) * n->t) / Dur + n->s8;

		if (n->f12) {
			x += off;
			off -= 32;
		}

		w = a->us14 + ((a->us16 - a->us14) * n->t) / Dur;
		Mess_DrawPanelText(y, x, n->text, w, 0, 256);
	}
}
