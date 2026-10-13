/* FUN_8005ffac (0x8005ffac) */
#include "names.h"
#include "game/padbuttons.h"

typedef struct {
	char p[0x184];
	int f184;
	int f188;
} G;

typedef struct {
	char p[0xf0];
	int f0;
	int f4;
	int f8;
	int fc;
} Z;

extern unsigned char Pad[];
extern G GPreferences;
extern Z GameText;

void FUN_800648c0(int, int, int);
int VerticalMenu_getSelected(int);
void SFX_PlayX(int, int, int);
void FUN_8008b900(char *, char *, int, int);
void VerticalMenu_setString(int, int, char *);
void Pad_ClearTriggers(unsigned char *);

int FUN_8005ffac(int a) {
	char buf[32];
	int s3, s1, t;
	unsigned char *v;
	unsigned char *s0;

	s0 = Pad;
	FUN_800648c0(a, 1, *(int *)(s0 + 0x2b8) == 0);

	if (s0[PAD_MENU_LEFT + PAD_TRIGGERED] || s0[PAD_SIZE + PAD_MENU_LEFT + PAD_TRIGGERED] ||
	    s0[PAD_MENU_RIGHT + PAD_TRIGGERED] || s0[PAD_SIZE + PAD_MENU_RIGHT + PAD_TRIGGERED]) {
		s3 = 0;

		if (Pad[PAD_MENU_LEFT + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_LEFT + PAD_TRIGGERED])
			s3 = 1;

		Pad_ClearTriggers(Pad);
		Pad_ClearTriggers(Pad + 364);

		if (VerticalMenu_getSelected(a) == 3) {
			if (s3 != GPreferences.f184)
				SFX_PlayX(144, 0x2000, 0);

			s1 = s3 != 0;
			GPreferences.f184 = s1;
			FUN_8008b900(buf, "%s %s", GameText.f8, s1 ? GameText.f0 : GameText.f4);
			VerticalMenu_setString(a, 3, buf);
		}

		if (VerticalMenu_getSelected(a) == 4) {
			s1 = s3 != 0;

			if (s3 != GPreferences.f188)
				SFX_PlayX(144, 0x2000, 0);

			GPreferences.f188 = s1;
			FUN_8008b900(buf, "%s %s", GameText.fc, s1 ? GameText.f0 : GameText.f4);
			VerticalMenu_setString(a, 4, buf);
		}
	}

	if (Pad[PAD_MENU_START + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] ||
	    Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_ACCEPT + PAD_TRIGGERED]) {
		t = VerticalMenu_getSelected(a);

		if (t < 5) {
			if (t >= 3) {
				Pad_ClearTriggers(Pad);
				Pad_ClearTriggers(Pad + 364);
			}
		}
	}
}
