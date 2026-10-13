/* FUN_8005e4ec (0x8005e4ec) */
#include "names.h"
#include "game/padbuttons.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
	short off2;
	short pad3;
	void (*fn2)(char *);
};

typedef struct {
	struct VT *vt;
} O;

extern unsigned char Pad[];

void Pad_ClearTriggers(unsigned char *);
O *__nw__6CClassUi(int);
O *FUN_800652e4(O *, int, int, int, int, int, int, int, int);
O *PadBuffer_ctor(O *, int);
void FrontLoader_Continue(void);
void Pad_Update(void);
void PollHost(void);
void HiRes_ClearOTag(void);
void ResetOT(void);
void HiRes_DrawOTag(void);
void SFX_PlayX(int, int, int);
void VerticalMenu_erase(O *);

int FUN_8005e4ec(int a, int b, int c) {
	int r, i;
	O *m;
	O *n;

	r = 0;
	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + 364);

	m = __nw__6CClassUi(820);

	if (m)
		m = FUN_800652e4(m, 100, 398, 15, a, b, 0, 0, 0);

	n = __nw__6CClassUi(284);

	if (n)
		n = PadBuffer_ctor(n, -1);

	do {
		FrontLoader_Continue();
		Pad_Update();
		PollHost();
		HiRes_ClearOTag();
		ResetOT();
		m->vt->fn2((char *)m + m->vt->off2);
		HiRes_DrawOTag();

		if (Pad[PAD_MENU_START + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] ||
		    Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] ||
		    Pad[PAD_SIZE + PAD_MENU_ACCEPT + PAD_TRIGGERED] || Pad[PAD_MENU_BACK + PAD_TRIGGERED] ||
		    Pad[PAD_SIZE + PAD_MENU_BACK + PAD_TRIGGERED]) {
			Pad_ClearTriggers(Pad);
			Pad_ClearTriggers(Pad + 364);
			SFX_PlayX(145, 0x2000, 0);

			r = -1;
		}

		if (c != -1) {
			c--;

			if (c < 0)
				r = -1;
		}
	} while (r == 0);

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();
		VerticalMenu_erase(m);
		HiRes_DrawOTag();
	} while (i < 2);

	if (m)
		m->vt->fn((char *)m + m->vt->off, 3);

	if (n)
		n->vt->fn((char *)n + n->vt->off, 3);

	return r;
}
