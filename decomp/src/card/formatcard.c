/* Card_FormatCard__Fii (0x80015474) */
#include "names.h"
extern char s_bu_1d_1d_8009e124[];

void FUN_8008b900(char *, char *, int, int);
void _clear_event_x(void);
int FUN_8008eaa8(char *);
int _card_event_x(void);

int Card_FormatCard(int a, int b) {
	char buf[8];
	int s, r;

	FUN_8008b900(buf, s_bu_1d_1d_8009e124, a, b);
	_clear_event_x();

	s = FUN_8008eaa8(buf);
	r = _card_event_x();

	if (r == 1)
		r = 1;
	else if (r == 3)
		r = -1;
	else
		r = -3;

	if (s == 0 && r == 1)
		r = -3;

	return r;
}
