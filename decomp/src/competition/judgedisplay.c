/* Competition_JudgeDisplay__Fv (0x800747b0) */
#include "names.h"

typedef struct {
	char _0[16];
	unsigned i16;
	char _14[12];
	int i32;
	char _24[44];
	short x[5];
	unsigned char b90, b91;
	char _5c[100];
	unsigned char b192;
} J;

extern J theCompetition;
extern char *GameText_180[];

void Mess_DeleteAll(void);
void Mess_SetScale(int);
void Mess_SetTextJustify(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
int FUN_8008b900(char *, char *, ...);
int Mess_DrawText(int, int, char *, int, int, int, int);
void SFX_PlayX(int, int, int);
void Front_PrintCentredPrompt(char *);

void Competition_JudgeDisplay(void) {
	char buf[16];
	J *p = &theCompetition;
	int i, t, c, sum, k;
	unsigned s;

	Mess_DeleteAll();
	Mess_SetScale(256);
	Mess_SetTextJustify(0);
	Mess_SetRGB(182, 132, 36, 0);
	FUN_8008b900(buf, "HEAT %d OVER!", p->b192 + 1);
	Mess_DrawText(256, 40, buf, 0, 0, 256, 0);
	Mess_DrawText(256, 70, "JUDGES SCORES", 0, 0, 256, 0);
	Mess_SetRGB(122, 122, 122, 0);
	Mess_SetTextJustify(1);

	t = 0;

	if (p->i16 >= 76)
		t = (p->i16 - 75) << 2;

	if (t > 100)
		t = 100;

	c = (122 - t) & 255;

	for (i = 0; i < 5; i++) {
		s = i * 15;

		if (p->i16 == s + 1)
			SFX_PlayX(52, 8192, 0);

		if (s < p->i16) {
			FUN_8008b900(buf, "JUDGE %d     %d.%d", i + 1, p->x[i] / 10, p->x[i] % 10);
		} else {
			FUN_8008b900(buf, "JUDGE %d", i + 1);
		}

		if (i == p->b91 || i == p->b90)
			Mess_SetRGB(c, c, c, 0);
		else
			Mess_SetRGB(122, 122, 122, 0);

		Mess_DrawText(150, i * 20 + 90, buf, 0, 0, 256, 0);
	}

	if (t >= 100) {
		if (p->i32 == 0)
			SFX_PlayX(51, 8192, 0);

		Mess_SetTextJustify(0);
		Mess_SetRGB(182, 132, 36, 0);

		sum = 0;

		for (k = 0; k < 5; k++)
			if (k != p->b91 && k != p->b90)
				sum += p->x[k];

		sum = sum / 3;
		Mess_SetTextJustify(0);
		FUN_8008b900(buf, "HEAT %d SCORE - ", p->b192 + 1);
		Mess_DrawText(200, 195, buf, 0, 0, 256, 0);
		FUN_8008b900(buf, "%d.%d", sum / 10, sum % 10);
		Mess_SetScale(384);
		Mess_DrawText(335, 195, buf, 0, 0, 256, 0);

		p->i32 = 1;
	} else {
		p->i32 = 0;
	}

	Mess_SetScale(256);
	Front_PrintCentredPrompt(GameText_180[0]);
}
