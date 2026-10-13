/* Mess_TextHeightBelowBaseline__FPCc (0x8001db80) */
#include "names.h"

typedef struct {
	int **p;
	char pad[32];
} A;

typedef struct {
	char pad[20];
	int *p;
	char pad2[2];
	unsigned short h;
	char pad3[8];
} B;

extern A Font_table_14[];
extern B Font_table[];

unsigned char NewFont;
int Current_font;

int FUN_8001d548(unsigned char *s);
int GetCharIndex(int c);

int Mess_TextHeightBelowBaseline(unsigned char *s) {
	int m;
	int v;
	int c;
	unsigned char *p;

	if (!NewFont)
		return FUN_8001d548(s);

	m = 0;
	p = s;

	while (*p) {
		c = GetCharIndex(*p);

		if ((unsigned)(c - 250) < 5)
			v = 0;
		else if (c == 255)
			v = 0;
		else
			v = *(int *)((char *)(Font_table_14[Current_font].p) + c * 36 + 4) -
			    *(int *)((char *)(Font_table_14[Current_font].p) + c * 36 + 16);

		if (m < v)
			m = v;

		p++;
	}

	return (m * Font_table[Current_font].h) >> 8;
}
