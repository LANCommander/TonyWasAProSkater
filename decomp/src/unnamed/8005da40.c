/* FUN_8005da40 (0x8005da40) */
#include "names.h"
#include "game/padbuttons.h"
struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

typedef struct {
	struct VT *vt;
} O;

extern unsigned char Pad[];
extern char DAT_800c0a79[];
extern int DAT_800bf0dc[], DAT_800bf0e8[];
extern char s_ingame1_fnt_800a1b84[], s_Loading_Memory_Card_Data[];
extern int GameFade, DemoTimer;

int DAT_800d1ce8, DAT_800d1cec, BackOut, DAT_800d1d14;
O *DAT_800d1d0c;

void FUN_8008c468(int);
void FUN_8006b2f4(void);
void FUN_8006b240(void);
void FUN_80061ea0(void);
void FUN_800242cc(void);
void SetClippingBox(int, int, int, int);
void Sprite_draw(O *, int, int, int, int, int, int);
void FUN_80070ff4(void);
int FUN_800718d0(void);
void SFX_PlayX(int, int, int);
void Pause__Fi(int);
void Pad_ClearTriggers(unsigned char *);
void Mess_LoadFont__FPciii(char *, int, int, int);
void Mess_SetTextJustify(int);
void Mess_SetScale(int);
void Mess_SetRGB(int, int, int, int);
void Mess_DrawText(int, int, char *, int, int, int, int);
void FUN_8006b26c(void);
void Mess_UnloadFont(void);

void FUN_8005da40(void) {
	FUN_8008c468(0);
	FUN_8006b2f4();
	FUN_8006b240();
	FUN_80061ea0();
	FUN_800242cc();
	SetClippingBox(0, 0, 512, 240);

	if (DAT_800d1d14) {
		DAT_800d1d14 = 0;
		Sprite_draw(DAT_800d1d0c, DAT_800bf0dc[0], DAT_800bf0e8[0], 0, -1, -1, -1);
	}

	FUN_80070ff4();

	if (FUN_800718d0())
#ifdef PC_PORT
		Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] = 1;
#else
		DAT_800c0a79[0] = 1; /* Pad[0].X.Triggered */
#endif

	if (Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] && DemoTimer == 0) {
		SFX_PlayX(2, 0x2000, 0);
		Pause__Fi(20);
		Pad_ClearTriggers(Pad);
		Pad_ClearTriggers(Pad + 364);

		DAT_800d1ce8 = 0;
		BackOut = 1;
	}

	if (DAT_800d1cec) {
		if (DAT_800d1d0c)
			DAT_800d1d0c->vt->fn((char *)DAT_800d1d0c + DAT_800d1d0c->vt->off, 3);

		if (DemoTimer == 0) {
			GameFade = 64;
			return;
		}

		Mess_LoadFont__FPciii(s_ingame1_fnt_800a1b84, -1, -1, -1);
		FUN_8006b240();
		FUN_80061ea0();
		Mess_SetTextJustify(0);
		Mess_SetScale(255);
		Mess_SetRGB(150, 100, 25, 0);
		Mess_DrawText(256, 110, s_Loading_Memory_Card_Data, 1, 0, 256, 0);
		Mess_DrawText(256, 130, s_Loading_Memory_Card_Data + 32, 1, 0, 256, 0);
		FUN_8006b26c();
		Mess_UnloadFont();
	}
}
