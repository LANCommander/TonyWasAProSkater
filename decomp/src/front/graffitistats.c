/* Graffiti_Stats__FP6CBruceiiiiiiiii (0x80017abc) */
#include "names.h"

typedef struct {
	char _0[496];
	int f496, f500;
} CBruce;

extern char s_TAGS[], s_STEALS[], s_HIGH[], s_WINS[], s_LOSSES[], s_STREAK[], s_d[];

void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetTextJustify(int);
int Mess_DrawText(int, int, char *, int, int, int, int);
int FUN_8008b900(char *, char *, int);

void Graffiti_Stats(CBruce *b, int x, int y, int r, int g, int bl, int v1, int v2, int v3, int v4) {
	char buf[16];

	Mess_SetRGB(r, g, bl, 0);
	Mess_SetTextJustify(1);
	Mess_DrawText(x + 20, y, s_TAGS, 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 17, s_STEALS, 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 51, s_HIGH, 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 68, s_WINS, 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 85, s_LOSSES, 0, 0, 256, 0);
	Mess_DrawText(x + 20, y + 102, s_STREAK, 0, 0, 256, 0);
	Mess_SetTextJustify(2);
	FUN_8008b900(buf, s_d, b->f496);
	Mess_DrawText(x + 136, y, buf, 0, 0, 256, 0);
	FUN_8008b900(buf, s_d, b->f500);
	Mess_DrawText(x + 136, y + 17, buf, 0, 0, 256, 0);
	FUN_8008b900(buf, s_d, v4);
	Mess_DrawText(x + 136, y + 51, buf, 0, 0, 256, 0);
	FUN_8008b900(buf, s_d, v1);
	Mess_DrawText(x + 136, y + 68, buf, 0, 0, 256, 0);
	FUN_8008b900(buf, s_d, v2);
	Mess_DrawText(x + 136, y + 85, buf, 0, 0, 256, 0);
	FUN_8008b900(buf, s_d, v3);
	Mess_DrawText(x + 136, y + 102, buf, 0, 0, 256, 0);
}
