/* Horse_DisplayWords__FPi (0x8001631c) */
#include "names.h"

void Mess_SetScale(int);
void Mess_SetTextJustify(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_DrawText(int, int, char *, int, int, int, int);

extern char GCareer[];

void Horse_DisplayWords(int *p) {
	char buf[32];
	int i;

	Mess_SetScale(256);
	Mess_SetTextJustify(0);

	for (i = 0; i < 2; i++) {
		char *s = GCareer + 0x9c0; /* Options.HorseWord */
		char *o = buf;
		int n = p[i];

		if (*s) {
			do {
				if (n) {
					n--;
					*o++ = *s++;
				} else {
					*o++ = '-';
					s++;
				}
			} while (*s);
		}

		*o = 0;

		if (i == 0) {
			Mess_SetRGB(80, 80, 128, 0);
			Mess_DrawText(86, 32, "PLAYER 1", 0, 0, 256, 0);
		} else {
			Mess_SetRGB(128, 80, 80, 0);
			Mess_DrawText(426, 32, "PLAYER 2", 0, 0, 256, 0);
		}

		Mess_DrawText(426 - (1 - i) * 340, 48, buf, 0, 0, 256, 0);
	}
}
