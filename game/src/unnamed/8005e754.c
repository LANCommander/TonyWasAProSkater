/* FUN_8005e754 (0x8005e754) */
#include "names.h"
#include "game/padbuttons.h"

typedef struct {
	char *a, *b, *c, *d, *e, *f;
} S;

struct VerticalMenu;

extern unsigned char Pad[];
extern S DAT_800a1c44;

int DAT_800d1d2c;

int Pad_Update(void);
int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);
int FUN_8005e8e0(int);
int FUN_8005ea00(int);
int FUN_80060274(int);
int FUN_800604c8(int);
int FUN_80060854(int);
void FUN_8005e728(struct VerticalMenu *);

/* PC: QUIT after VIEW VIDEOS. Its id 7 is shared with the hidden cheat menu, which is only
   reached through the menu path in a (FUN_8005e09c), so a 7 picked by hand means quit. */
void PC_RequestQuit(void);

static char s_quit[] = "QUIT";

int FUN_8005e754(int a) {
	char *t[7];
	int k[7];
	int r, i, path;

	Pad_Update();

	if (Pad[PAD_SPIN_LEFT + PAD_PRESSED] && Pad[PAD_SPIN_RIGHT + PAD_PRESSED])
		DAT_800d1d2c = 1;

	for (;;) {
		*(S *)t = DAT_800a1c44;
		t[6] = s_quit;

		for (i = 0; i < 7; i++)
			k[i] = 0;

		path = a != 0;
		r = Front_RunMenu(t, k, 7, 0, &a, FUN_8005e728, 0, 0, 0);

		if (r != -1) {
			switch (r) {
				case 1:
					r = FUN_8005e8e0(a);
					break;
				case 2:
					r = FUN_8005ea00(a);
					break;
				case 6:
					r = FUN_800604c8(a);
					break;
				case 7:
					if (path) {
						r = FUN_80060854(a);
					} else {
						PC_RequestQuit();

						r = -1;
					}

					break;
				case 5:
					r = FUN_80060274(a);
					break;
			}

			if (r == -1) {
				a = 0;
				continue;
			}
		}

		break;
	}

	return r;
}
