/* FUN_8005ffac (0x8005ffac) */
#include "names.h"
#include "game/padbuttons.h"

typedef struct {
	char p[0x184];
	int f184;
	int f188;
} G;

typedef struct {
	char *p[0xf0 / 4];
	char *f0;
	char *f4;
	char *f8;
	char *fc;
} Z;

struct VerticalMenu;

extern unsigned char Pad[];
extern G GPreferences;
extern Z GameText;

void FUN_800648c0(struct VerticalMenu *, int, int);
int VerticalMenu_getSelected(struct VerticalMenu *);
void SFX_PlayX(int, int, int);
void sprintf(char *, char *, char *, char *);
void VerticalMenu_setString(struct VerticalMenu *, int, char *);
void Pad_ClearTriggers(unsigned char *);

int FUN_8005ffac(struct VerticalMenu *a) {
	char buf[32];
	int s3, s1, t;
	unsigned char *v;
	unsigned char *s0;

	s0 = Pad;
	FUN_800648c0(a, 1, *(int *)(s0 + PAD_SIZE + 0x14c) == 0 /* Pad[1].Type */);

	if (s0[PAD_MENU_LEFT + PAD_TRIGGERED] || s0[PAD_SIZE + PAD_MENU_LEFT + PAD_TRIGGERED] ||
	    s0[PAD_MENU_RIGHT + PAD_TRIGGERED] || s0[PAD_SIZE + PAD_MENU_RIGHT + PAD_TRIGGERED]) {
		s3 = 0;

		if (Pad[PAD_MENU_LEFT + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_LEFT + PAD_TRIGGERED])
			s3 = 1;

		Pad_ClearTriggers(Pad);
		Pad_ClearTriggers(Pad + PAD_SIZE);

		if (VerticalMenu_getSelected(a) == 3) {
			if (s3 != GPreferences.f184)
				SFX_PlayX(144, 0x2000, 0);

			s1 = s3 != 0;
			GPreferences.f184 = s1;
			sprintf(buf, "%s %s", GameText.f8, s1 ? GameText.f0 : GameText.f4);
			VerticalMenu_setString(a, 3, buf);
		}

		if (VerticalMenu_getSelected(a) == 4) {
			s1 = s3 != 0;

			if (s3 != GPreferences.f188)
				SFX_PlayX(144, 0x2000, 0);

			GPreferences.f188 = s1;
			sprintf(buf, "%s %s", GameText.fc, s1 ? GameText.f0 : GameText.f4);
			VerticalMenu_setString(a, 4, buf);
		}
	}

	if (Pad[PAD_MENU_START + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] ||
	    Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_ACCEPT + PAD_TRIGGERED]) {
		t = VerticalMenu_getSelected(a);

		if (t < 5) {
			if (t >= 3) {
				Pad_ClearTriggers(Pad);
				Pad_ClearTriggers(Pad + PAD_SIZE);
			}
		}
	}
}
