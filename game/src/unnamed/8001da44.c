/* Mess_TextWidth__FPCc (0x8001da44) */
#include "names.h"

typedef struct E {
	char p0[0x14];
	struct E *f14;
	char p1[2];
	unsigned short f1a;
	int f1c;
	int f20;
} E;

extern E Font_table[];

int Current_font;
unsigned char NewFont;

int FUN_8001d340(unsigned char *);
int GetCharIndex(int);

int Mess_TextWidth(unsigned char *s0) {
	int n, idx;
	unsigned char *s;

	if (NewFont == 0)
		return FUN_8001d340(s0);

	s = s0;
	n = 0;

	if (*s) {
		do {
			idx = GetCharIndex(*s);

			if ((unsigned)(idx - 250) >= 5) {
				if (idx == 255) {
					n += ((Font_table[Current_font].f20 + Font_table[Current_font].f1c) *
					      Font_table[Current_font].f1a) >>
					     8;
				} else {
					n += (((Font_table[Current_font].f14 + idx)->f20 +
					       Font_table[Current_font].f1c) *
					      Font_table[Current_font].f1a) >>
					     8;
				}
			}

			s++;
		} while (*s);
	}

	return n;
}
