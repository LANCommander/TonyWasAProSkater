/* FUN_8005f6bc (0x8005f6bc) */
/* C++ (front end, THPS1-only; no demo twin). The CONTROLLER options screen (opt_ctr.bmp): for the
   player in DAT_800d1d28 it copies the pad mapping out of the options into the edit buffer
   DAT_800bf260, labels the four button rows with their current function (FUN_8005ea74), builds
   the "vibration on/off" and "Autokick on/off" rows, snapshots the mapping into DAT_800bf280 and
   runs the 8-entry menu (TRIANGLE, SQUARE, CIRCLE, X, VIBRATION, AUTOKICK, RESET TO DEFAULT,
   BACK TO OPTIONS; ids 1296..) with FUN_8005eb78 as the per-frame handler. Backing out restores
   the options background and returns -1. Called from the options menu FUN_80060274. */
#include "game/career.h"
#include "game/pad.h"
#include "game/front.h"

extern SOptions GPreferences;
extern SPadMap DAT_800bf260[2];
extern SPadMap DAT_800bf280[2];
extern LocalText GameText;
extern SLabels DAT_800a1d0c;
extern char s_opt_ctr_bmp[];
extern char s_options_bmp[];
extern char s_s_s[];

int DAT_800d1d28;

extern "C" {
	void FUN_8005c97c(int);
	void FUN_8005ca64(void);
	int FUN_8005ca3c(char *);
	void FUN_80061714(int);
	int FUN_8008b900(char *, char *, ...);
	char *FUN_8005ea74(int);
	int FUN_8005eb78(void);
	int FUN_8005e0e0(char **, int *, int, int, int *, int (*)(void), int *, int *, int);
}

#ifdef PC_PORT
/* PC: the controls screen. The PS1's (a pad picture with what each face button does, plus
   vibration and autokick) is replaced by a list of every action with its two bindings, for the
   keyboard or the gamepad (switched at the top; player 2 has a gamepad only). Accept on a binding
   waits for the new key or button, Delete (or X on a gamepad) clears it; an input does one thing
   per set, so taking one from another action swaps them (PC_InputBind). The bindings are saved
   to Settings.ini on the way out; autokick and vibration stay per player in the options as
   before. Drawn over options.bmp like the other front-end screens, as its own loop: every frame
   the list area is put back from a copy of the background (in the frame end, pc_hires_before_draw,
   so it is never shown without its text), then the text is drawn. */
#include <stdlib.h>
#include "game/shellutils.h"
#include "pc_input.h"

extern SControl Pad[2];
extern int DemoTimer;
extern int DAT_800d1cb8; /* soft reset requested (FUN_8005c68c) */

int SFX_PlayX(unsigned, short, int);
int Mess_GetFontScale();
void Mess_SetFontScale(uint);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);

extern "C" {
	void Mess_SetTextJustify__FUc(int);
	void Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int);
	void FUN_8005c780(void);
	void FUN_8005c934(void);
	void FUN_80070ff4(void); /* Pad_Update */
	void FUN_8005c68c(void);
	void FUN_800242cc(void);
	void FUN_8006b240(void);
	void FUN_80061ea0(void);
	void FUN_8006b26c(void);
	extern void (*pc_hires_before_draw)(void);
}

enum { ROW_DEVICE, ROW_AUTOKICK, ROW_VIBRATION, ROW_HEADER, ROW_ACTION, ROW_RESET, ROW_BACK };

struct ControlsRow {
	uchar kind;
	uchar action; /* PcAction for ROW_ACTION */
	const char *text;
};

static const ControlsRow s_rows[] = {
	{ROW_DEVICE, 0, 0},
	{ROW_AUTOKICK, 0, 0},
	{ROW_VIBRATION, 0, 0},
	{ROW_HEADER, 0, "SKATING"},
	{ROW_ACTION, PC_ACT_UP, 0},
	{ROW_ACTION, PC_ACT_DOWN, 0},
	{ROW_ACTION, PC_ACT_LEFT, 0},
	{ROW_ACTION, PC_ACT_RIGHT, 0},
	{ROW_ACTION, PC_ACT_OLLIE, 0},
	{ROW_ACTION, PC_ACT_FLIP, 0},
	{ROW_ACTION, PC_ACT_GRAB, 0},
	{ROW_ACTION, PC_ACT_GRIND, 0},
	{ROW_ACTION, PC_ACT_SPIN_LEFT, 0},
	{ROW_ACTION, PC_ACT_SPIN_RIGHT, 0},
	{ROW_ACTION, PC_ACT_TAP_SPIN_LEFT, 0},
	{ROW_ACTION, PC_ACT_TAP_SPIN_RIGHT, 0},
	{ROW_ACTION, PC_ACT_CAMERA, 0},
	{ROW_ACTION, PC_ACT_PAUSE, 0},
	{ROW_HEADER, 0, "MENUS"},
	{ROW_ACTION, PC_ACT_MENU_UP, 0},
	{ROW_ACTION, PC_ACT_MENU_DOWN, 0},
	{ROW_ACTION, PC_ACT_MENU_LEFT, 0},
	{ROW_ACTION, PC_ACT_MENU_RIGHT, 0},
	{ROW_ACTION, PC_ACT_MENU_ACCEPT, 0},
	{ROW_ACTION, PC_ACT_MENU_BACK, 0},
	{ROW_RESET, 0, "RESET TO DEFAULT"},
	{ROW_BACK, 0, "BACK TO OPTIONS"},
};

#define NUM_ROWS ((int)(sizeof s_rows / sizeof s_rows[0]))
#define VISIBLE_ROWS 13
#define ROW_TOP 70     /* centre of the first visible row */
#define ROW_HEIGHT 24
#define NAME_X 130     /* action names, left-justified */
#define SLOT_X0 400    /* the two binding columns, centred */
#define SLOT_X1 535
#define HINT_Y 406
#define CAPTURE_FRAMES 300

/* (this font has letters and digits but no '.', '-' or ':', so the screen's own text is words) */

/* the area redrawn every frame (640x480 front-end coordinates = VRAM), and the background there */
static RECT s_area = {110, 40, 520, 380};
static ulong *s_saved;

static void RestoreArea(void) { LoadImage(&s_area, s_saved); }

static void SetColour(int selected, int dim) {
	if (selected)
		Mess_SetRGB(182, 132, 36, 0);
	else if (dim)
		Mess_SetRGB(72, 72, 72, 0);
	else
		Mess_SetRGB(122, 122, 122, 0);
}

static void Text(int x, int y, int justify, const char *s) {
	Mess_SetTextJustify__FUc(justify);
	Mess_DrawText__FiiPCciUiib(x, y, (char *)s, 0, 0, 256, 0);
}

/* Up/down with key repeat (a press, then every 4 frames after 18 held) */
static int Repeat(SButton *b) {
	if (b->Triggered) {
		b->Triggered = 0;
		return 1;
	}

	return b->Pressed && b->PressedTime > 18 && (b->PressedTime & 3) == 0;
}

static int Selectable(int row) { return s_rows[row].kind != ROW_HEADER; }

static int AnyMenuButton(SControl *p) {
	return p->MenuUp.Pressed || p->MenuDown.Pressed || p->MenuLeft.Pressed || p->MenuRight.Pressed ||
	       p->MenuAccept.Pressed || p->MenuBack.Pressed;
}

extern "C" int FUN_8005f6bc(int a) {
	int pl = DAT_800d1d28;
	int gamepad = pl != 0 || PC_InputGamepadConnected(0);
	int sel = 0, top = 0, slot = 0;
	int capturing = 0, settling = 0, dirty = 0, result = -1;
	int base_scale = Mess_GetFontScale();
	int small_scale = base_scale * 3 / 4;
	(void)a;
	FUN_8005c97c(11);
	FUN_8005ca64();
	FUN_80061714(FUN_8005ca3c(s_options_bmp));
	DrawSync(0);
	s_saved = (ulong *)malloc(s_area.w * s_area.h * 2);
	StoreImage(&s_area, s_saved);
	DrawSync(0);
	pc_hires_before_draw = RestoreArea;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
	DemoTimer = 1600;

	for (;;) {
		SControl *p = &Pad[pl];
		char buf[64];
		int i;

		FUN_8005c780();
		FUN_8005c934();
		FUN_80070ff4();
		FUN_8005c68c();

		if (DAT_800d1cb8) {
			result = 0x666;
			break;
		}

		FUN_800242cc();
		FUN_8006b240();
		FUN_80061ea0();

		/* ---- input */
		if (capturing) {
			int in = PC_InputCapture(pl, gamepad);

			if (in != PC_BIND_NONE) {
				PC_InputBind(pl, gamepad, (PcAction)s_rows[sel].action, slot, in);
				dirty = 1;
				capturing = 0;
				settling = 1;
				SFX_PlayX(145, 8192, 0);
			} else if (--capturing == 0) {
				SFX_PlayX(147, 8192, 0);
			}

			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		} else if (settling) {
			/* the key just bound may now be a menu button: wait until it is let go */
			settling = AnyMenuButton(p);
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		} else {
			int kind = s_rows[sel].kind;
			int left = p->MenuLeft.Triggered, right = p->MenuRight.Triggered;

			if (Repeat(&p->MenuUp)) {
				do
					sel = sel ? sel - 1 : NUM_ROWS - 1;
				while (!Selectable(sel));

				SFX_PlayX(144, 8192, 0);
			}

			if (Repeat(&p->MenuDown)) {
				do
					sel = sel == NUM_ROWS - 1 ? 0 : sel + 1;
				while (!Selectable(sel));

				SFX_PlayX(144, 8192, 0);
			}

			if (left || right) {
				if (kind == ROW_DEVICE && pl == 0) {
					gamepad = right;
					SFX_PlayX(144, 8192, 0);
				} else if (kind == ROW_AUTOKICK) {
					GPreferences.AutoKickEnabled[pl] = left;
					SFX_PlayX(144, 8192, 0);
				} else if (kind == ROW_VIBRATION) {
					GPreferences.VibrationEnabled[pl] = left;
					SFX_PlayX(144, 8192, 0);
				} else if (kind == ROW_ACTION) {
					slot = right;
					SFX_PlayX(144, 8192, 0);
				}
			}

			if (p->MenuAccept.Triggered) {
				switch (kind) {
					case ROW_DEVICE:
						if (pl == 0)
							gamepad ^= 1;
						SFX_PlayX(144, 8192, 0);
						break;
					case ROW_AUTOKICK:
						GPreferences.AutoKickEnabled[pl] ^= 1;
						SFX_PlayX(144, 8192, 0);
						break;
					case ROW_VIBRATION:
						GPreferences.VibrationEnabled[pl] ^= 1;
						SFX_PlayX(144, 8192, 0);
						break;
					case ROW_ACTION:
						if (gamepad && !PC_InputGamepadConnected(pl)) {
							SFX_PlayX(147, 8192, 0);
						} else {
							capturing = CAPTURE_FRAMES;
							SFX_PlayX(145, 8192, 0);
						}
						break;
					case ROW_RESET:
						PC_InputResetDevice(pl, gamepad);
						dirty = 1;
						SFX_PlayX(145, 8192, 0);
						break;
					case ROW_BACK:
						SFX_PlayX(147, 8192, 0);
						break;
				}

				if (kind == ROW_BACK)
					break;
			}

			if (kind == ROW_ACTION && PC_InputClearPressed(pl) &&
			    PC_InputGetBind(pl, gamepad, (PcAction)s_rows[sel].action, slot) != PC_BIND_NONE) {
				PC_InputBind(pl, gamepad, (PcAction)s_rows[sel].action, slot, PC_BIND_NONE);
				dirty = 1;
				SFX_PlayX(147, 8192, 0);
			}

			if (p->MenuBack.Triggered) {
				SFX_PlayX(147, 8192, 0);
				break;
			}

			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		}

		/* keep the selection on screen (and a set's header above its first row) */
		if (sel < top)
			top = sel > 0 && !Selectable(sel - 1) ? sel - 1 : sel;

		if (sel >= top + VISIBLE_ROWS)
			top = sel - VISIBLE_ROWS + 1;

		if (sel <= 1)
			top = 0;

		/* ---- draw */
		Mess_SetFontScale(small_scale);

		for (i = 0; i < VISIBLE_ROWS && top + i < NUM_ROWS; i++) {
			const ControlsRow *r = &s_rows[top + i];
			int y = ROW_TOP + i * ROW_HEIGHT + 7;
			int on = top + i == sel;

			switch (r->kind) {
				case ROW_DEVICE:
					SetColour(on && !gamepad, gamepad || pl != 0);
					Text(SLOT_X0 - 60, y, 0, "KEYBOARD");
					SetColour(on && gamepad, !gamepad);
					Text(SLOT_X1 - 60, y, 0, "GAMEPAD");
					break;
				case ROW_AUTOKICK:
				case ROW_VIBRATION: {
					int value = r->kind == ROW_AUTOKICK ? GPreferences.AutoKickEnabled[pl]
					                                    : GPreferences.VibrationEnabled[pl];

					FUN_8008b900(buf, s_s_s,
					             r->kind == ROW_AUTOKICK ? GameText.pAutoKick : GameText.pVibration,
					             value ? GameText.pBooleanOn : GameText.pBooleanOff);
					SetColour(on, 0);
					Text(NAME_X, y, 1, buf);
					break;
				}
				case ROW_HEADER:
					Mess_SetRGB(70, 100, 140, 0);
					Text(NAME_X - 6, y, 1, r->text);
					break;
				case ROW_ACTION: {
					int s;

					SetColour(on, 0);
					Text(NAME_X, y, 1, PC_InputActionLabel((PcAction)r->action));

					for (s = 0; s < PC_BIND_SLOTS; s++) {
						int in = PC_InputGetBind(pl, gamepad, (PcAction)r->action, s);
						const char *label = in == PC_BIND_NONE ? "NONE" : PC_InputInputLabel(gamepad, in);

						if (on && s == slot && capturing)
							label = (capturing >> 4) & 1 ? "" : "PRESS";

						SetColour(on && s == slot, in == PC_BIND_NONE);
						Text(s ? SLOT_X1 : SLOT_X0, y, 0, label);
					}
					break;
				}
				default:
					SetColour(on, 0);
					Text(NAME_X, y, 1, r->text);
					break;
			}
		}

		Mess_SetRGB(70, 100, 140, 0);

		if (top > 0)
			Text(600, ROW_TOP - 14, 2, "MORE");

		if (top + VISIBLE_ROWS < NUM_ROWS)
			Text(600, ROW_TOP + (VISIBLE_ROWS - 1) * ROW_HEIGHT + 7 + 22, 2, "MORE");

		if (capturing)
			FUN_8008b900(buf, s_s_s, gamepad ? "PRESS A BUTTON FOR" : "PRESS A KEY FOR",
			             PC_InputActionLabel((PcAction)s_rows[sel].action));
		else if (s_rows[sel].kind == ROW_ACTION)
			FUN_8008b900(buf, s_s_s, "ACCEPT TO CHANGE", "   DELETE OR X TO CLEAR");
		else if (s_rows[sel].kind == ROW_DEVICE && pl == 0)
			FUN_8008b900(buf, s_s_s, "LEFT OR RIGHT", "TO SWITCH DEVICE");
		else
			buf[0] = 0;

		Mess_SetRGB(122, 122, 122, 0);
		Text(370, HINT_Y, 0, buf);
		Mess_SetFontScale(base_scale);
		FUN_8006b26c();
	}

	if (dirty)
		PC_InputSaveSettings();

	/* leave the options background clean for the options menu */
	pc_hires_before_draw = 0;
	LoadImage(&s_area, s_saved);
	DrawSync(0);
	free(s_saved);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	return result;
}
#else
extern "C" int FUN_8005f6bc(int a) {
	int pl = DAT_800d1d28;
	SLabels labels;
	int disabled[8];
	int xs[8];
	int ys[8];
	char vib[32];
	char ak[32];
	int r;

	FUN_8005c97c(13);
	FUN_8005ca64();
	FUN_80061714(FUN_8005ca3c(s_opt_ctr_bmp));

	labels = DAT_800a1d0c;
	disabled[0] = 0;
	disabled[1] = 0;
	disabled[2] = 0;
	disabled[3] = 0;
	disabled[4] = 0;
	disabled[5] = 0;
	disabled[6] = 0;
	disabled[7] = 0;
	xs[0] = 487;
	ys[0] = 95;
	xs[1] = 224;
	ys[1] = 126;
	xs[2] = 493;
	ys[2] = 238;
	xs[3] = 254;
	ys[3] = 271;
	xs[4] = 350;
	ys[4] = 315;
	xs[5] = 350;
	ys[5] = 345;
	xs[6] = 350;
	ys[6] = 375;
	xs[7] = 350;
	ys[7] = 405;
	DAT_800bf260[pl].Function[0] = GPreferences.ControllerFunction[pl][0];
	DAT_800bf260[pl].Function[1] = GPreferences.ControllerFunction[pl][1];
	DAT_800bf260[pl].Function[2] = GPreferences.ControllerFunction[pl][2];
	DAT_800bf260[pl].Function[3] = GPreferences.ControllerFunction[pl][3];
	labels.Name[0] = FUN_8005ea74(DAT_800bf260[pl].Function[0]);
	labels.Name[1] = FUN_8005ea74(DAT_800bf260[pl].Function[1]);
	labels.Name[2] = FUN_8005ea74(DAT_800bf260[pl].Function[2]);
	labels.Name[3] = FUN_8005ea74(DAT_800bf260[pl].Function[3]);

	FUN_8008b900(vib, s_s_s, GameText.pVibration,
	             GPreferences.VibrationEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);

	labels.Name[4] = vib;

	FUN_8008b900(ak, s_s_s, GameText.pAutoKick,
	             GPreferences.AutoKickEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);

	labels.Name[5] = ak;
	DAT_800bf280[pl].Function[0] = DAT_800bf260[pl].Function[0];
	DAT_800bf280[pl].Function[2] = DAT_800bf260[pl].Function[2];
	DAT_800bf280[pl].Function[1] = DAT_800bf260[pl].Function[1];
	DAT_800bf280[pl].Function[3] = DAT_800bf260[pl].Function[3];
	r = FUN_8005e0e0(labels.Name, disabled, 8, 1296, &a, FUN_8005eb78, xs, ys, 0);

	if (r == -1 || r == 1304) {
		FUN_8005c97c(11);
		FUN_8005ca64();
		FUN_80061714(FUN_8005ca3c(s_options_bmp));

		r = -1;
	}

	return r;
}
#endif
