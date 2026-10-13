/* Front_RunMenu__FPPcPiiiT1PFP12VerticalMenu_vT1T1i (0x8005e0e0) */
#include "names.h"
#include "game/padbuttons.h"

struct VT {
	char pad[8];
	short off;
	short p;
	void (*fn)(char *, int);
	short off2;
	short p2;
	void (*fn2)(char *);
};

typedef struct O {
	struct VT *vt;
} O;

extern char Pad[];
extern int DemoTimer;
extern int DAT_800d1cb8;

int DAT_800d2358, DAT_800d235c, DAT_800d1d28;

extern int Front_ControllerOptionsHandler();
void PollHost(void);
void SFX_PlayX(int, int, int);
void Front_CheckSoftReset(void);
void Front_UpdateMusicLoop(void);
void FrontLoader_Continue(void);
int FUN_8005e09c(int);
void ResetOT(void);
O *PadBuffer_ctor(O *, int);
void PadBuffer_update(O *);
int PadBuffer_getPress(O *);
O *VerticalMenu_ctor(O *, int, int, int, int *, int, int, int, int);
void VerticalMenu_setTriangles(O *, int, int);
void FUN_800648c0(O *, int, int);
int VerticalMenu_getSelected(O *);
void HiRes_ClearOTag(void);
void HiRes_DrawOTag(void);
void Pad_Update(void);
int strcmp(int, char *);
void Pad_ClearTriggers(char *);
O *__nw__6CClassUi(int);
void VerticalMenu_down(O *);
void VerticalMenu_erase(O *);
void VerticalMenu_up(O *);

int Front_RunMenu(int *a0, int *a1, int a2, int a3, int a4, void (*a5)(O *), int a6, int a7,
                  int a8) {
	O *s2;
	O *s1;
	int s3, s0, i, r, t;
	char *P;

	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + 364);

	s3 = FUN_8005e09c(a4);

	if (s3)
		return s3 + a3;

	s2 = __nw__6CClassUi(820);

	if (s2)
		s2 = VerticalMenu_ctor(s2, 100, 398, 15, a0, a2, a6, a7, a8);

	VerticalMenu_setTriangles(s2, DAT_800d2358, DAT_800d235c);

	for (i = 0; i < a2; i++) {
		if (a1[i])
			FUN_800648c0(s2, i, 1);
	}

	s0 = -1;

	if (a5 && (int)a5 == (int)Front_ControllerOptionsHandler)
		s0 = DAT_800d1d28;

	s1 = __nw__6CClassUi(284);

	if (s1)
		s1 = PadBuffer_ctor(s1, s0);

	DemoTimer = 1600;

	while (s3 == 0) {
		Front_UpdateMusicLoop();

		if (strcmp(*a0, "CAREER MODE") == 0) {
			if (DemoTimer)
				DemoTimer--;
			else
				s3 = 128;
		}

		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			s3 = 0x666;
			break;
		}

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

		r = PadBuffer_getPress(s1);

		if (r == 3) {
			SFX_PlayX(144, 8192, 0);
			VerticalMenu_up(s2);
		}

		if (r == 4) {
			SFX_PlayX(144, 8192, 0);
			VerticalMenu_down(s2);
		}

		if (r)
			DemoTimer = 1600;

		PadBuffer_update(s1);

		if (a5)
			a5(s2);

		s2->vt->fn2((char *)s2 + s2->vt->off2);
		HiRes_DrawOTag();

		if (Pad[PAD_MENU_START + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] ||
		    Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] ||
		    Pad[PAD_SIZE + PAD_MENU_ACCEPT + PAD_TRIGGERED]) {
			Pad_ClearTriggers(Pad);
			Pad_ClearTriggers(Pad + 364);
			SFX_PlayX(145, 8192, 0);

			t = VerticalMenu_getSelected(s2) + 1;
			s3 = a3 + t;
		}

		P = Pad;

		if (P[PAD_MENU_BACK + PAD_TRIGGERED] || P[PAD_SIZE + PAD_MENU_BACK + PAD_TRIGGERED]) {
			Pad_ClearTriggers(P);
			Pad_ClearTriggers(P + 364);
			SFX_PlayX(147, 8192, 0);

			s3 = -1;
		}
	}

	i = 0;

	do {
		i++;
		HiRes_ClearOTag();
		ResetOT();
		VerticalMenu_erase(s2);
		HiRes_DrawOTag();
	} while (i < 2);

	if (s2)
		s2->vt->fn((char *)s2 + s2->vt->off, 3);

	if (s1)
		s1->vt->fn((char *)s1 + s1->vt->off, 3);

	return s3;
}
