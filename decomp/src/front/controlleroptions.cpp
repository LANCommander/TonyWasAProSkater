/* Front_ControllerOptions__Fi (0x8005f6bc) */
/* C++ (front end, THPS1-only; no demo twin). The CONTROLLER options screen (opt_ctr.bmp): for the
   player in DAT_800d1d28 it copies the pad mapping out of the options into the edit buffer
   DAT_800bf260, labels the four button rows with their current function (Front_GetPadFunctionName), builds
   the "vibration on/off" and "Autokick on/off" rows, snapshots the mapping into DAT_800bf280 and
   runs the 8-entry menu (TRIANGLE, SQUARE, CIRCLE, X, VIBRATION, AUTOKICK, RESET TO DEFAULT,
   BACK TO OPTIONS; ids 1296..) with Front_ControllerOptionsHandler as the per-frame handler. Backing out restores
   the options background and returns -1. Called from the options menu FUN_80060274. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/career.h"
#include "game/pad.h"
#include "game/front.h"
#include "game/frontend.h"
#include "game/hires.h"
#include "game/main.h"
#include "game/mess.h"
#include "game/psyq.h"
#include "game/shellutils.h"

extern SOptions GPreferences;
extern SPadMap DAT_800bf260[2];
extern SPadMap DAT_800bf280[2];
extern LocalText GameText;
extern SLabels DAT_800a1d0c;

int DAT_800d1d28;

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
#include "pc_screen.h"

extern SControl Pad[2];
extern int DemoTimer;
extern int DAT_800d1cb8; /* soft reset requested (Front_CheckSoftReset) */

int SFX_PlayX(unsigned, short, int);
int Mess_GetFontScale();
void Mess_SetFontScale(uint);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);

extern "C" {
	extern void (*pc_hires_before_draw)(void);
}

enum { ROW_DEVICE, ROW_AUTOKICK, ROW_VIBRATION, ROW_HEADER, ROW_ACTION, ROW_RESET, ROW_BACK };

struct ControlsRow {
	uchar kind;
	uchar action; /* PcAction for ROW_ACTION */
	const char *text;
};

static const ControlsRow s_rows[] = {
	{ ROW_DEVICE, 0, 0 },
	{ ROW_AUTOKICK, 0, 0 },
	{ ROW_VIBRATION, 0, 0 },
	{ ROW_HEADER, 0, "SKATING" },
	{ ROW_ACTION, PC_ACT_UP, 0 },
	{ ROW_ACTION, PC_ACT_DOWN, 0 },
	{ ROW_ACTION, PC_ACT_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_OLLIE, 0 },
	{ ROW_ACTION, PC_ACT_FLIP, 0 },
	{ ROW_ACTION, PC_ACT_GRAB, 0 },
	{ ROW_ACTION, PC_ACT_GRIND, 0 },
	{ ROW_ACTION, PC_ACT_SPIN_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_SPIN_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_TAP_SPIN_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_TAP_SPIN_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_CAMERA, 0 },
	{ ROW_ACTION, PC_ACT_PAUSE, 0 },
	{ ROW_HEADER, 0, "MENUS" },
	{ ROW_ACTION, PC_ACT_MENU_UP, 0 },
	{ ROW_ACTION, PC_ACT_MENU_DOWN, 0 },
	{ ROW_ACTION, PC_ACT_MENU_LEFT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_RIGHT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_ACCEPT, 0 },
	{ ROW_ACTION, PC_ACT_MENU_BACK, 0 },
	{ ROW_RESET, 0, "RESET TO DEFAULT" },
	{ ROW_BACK, 0, "BACK TO OPTIONS" },
};

#define NUM_ROWS       ((int)(sizeof s_rows / sizeof s_rows[0]))
#define VISIBLE_ROWS   13
#define ROW_TOP        70 /* centre of the first visible row */
#define ROW_HEIGHT     24
#define NAME_X         130 /* action names, left-justified */
#define SLOT_X0        400 /* the two binding columns, centred */
#define SLOT_X1        535
#define HINT_Y         406
#define CAPTURE_FRAMES 300

/* (this font has letters and digits but no '.', '-' or ':', so the screen's own text is words) */

/* the area redrawn every frame (640x480 front-end coordinates = VRAM), and the background there */
static RECT s_area = { 110, 40, 520, 380 };
static ulong *s_saved;

static void RestoreArea(void) {
	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
}

static void SetColour(int selected, int dim) {
	if (selected)
		Mess_SetRGB(182, 132, 36, 0);
	else if (dim)
		Mess_SetRGB(72, 72, 72, 0);
	else
		Mess_SetRGB(122, 122, 122, 0);
}

static void Text(int x, int y, int justify, const char *s) {
	Mess_SetTextJustify(justify);
	Mess_DrawText(x, y, (char *)s, 0, 0, 256, 0);
}

/* Up/down with key repeat (a press, then every 4 frames after 18 held) */
static int Repeat(SButton *b) {
	if (b->Triggered) {
		b->Triggered = 0;
		return 1;
	}

	return b->Pressed && b->PressedTime > 18 && (b->PressedTime & 3) == 0;
}

static int Selectable(int row) {
	return s_rows[row].kind != ROW_HEADER;
}

static int AnyMenuButton(SControl *p) {
	return p->MenuUp.Pressed || p->MenuDown.Pressed || p->MenuLeft.Pressed ||
	       p->MenuRight.Pressed || p->MenuAccept.Pressed || p->MenuBack.Pressed;
}

int Front_ControllerOptions(int a) {
	int pl = DAT_800d1d28;
	int gamepad = pl != 0 || PC_InputGamepadConnected(0);
	int sel = 0, top = 0, slot = 0;
	int capturing = 0, settling = 0, dirty = 0, result = -1;
	int base_scale = Mess_GetFontScale();
	int small_scale = base_scale * 3 / 4;

	(void)a;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));
	DrawSync(0);

	s_saved = (ulong *)malloc(s_area.w * s_area.h * 2);

	PC_ScreenRead(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);

	pc_hires_before_draw = RestoreArea;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	DemoTimer = 1600;

	for (;;) {
		SControl *p = &Pad[pl];
		char buf[64];
		int i;

		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8) {
			result = 0x666;
			break;
		}

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

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

					sprintf(buf, "%s %s",
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
						const char *label =
						    in == PC_BIND_NONE ? "NONE" : PC_InputInputLabel(gamepad, in);

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
			sprintf(buf, "%s %s", gamepad ? "PRESS A BUTTON FOR" : "PRESS A KEY FOR",
			        PC_InputActionLabel((PcAction)s_rows[sel].action));
		else if (s_rows[sel].kind == ROW_ACTION)
			sprintf(buf, "%s %s", "ACCEPT TO CHANGE", "   DELETE OR X TO CLEAR");
		else if (s_rows[sel].kind == ROW_DEVICE && pl == 0)
			sprintf(buf, "%s %s", "LEFT OR RIGHT", "TO SWITCH DEVICE");
		else
			buf[0] = 0;

		Mess_SetRGB(122, 122, 122, 0);
		Text(370, HINT_Y, 0, buf);
		Mess_SetFontScale(base_scale);
		HiRes_DrawOTag();
	}

	if (dirty)
		PC_InputSaveSettings();

	/* leave the options background clean for the options menu */
	pc_hires_before_draw = 0;

	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);
	free(s_saved);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	return result;
}

/* PC: the graphics screen (Options > Graphics, from the options menu FUN_80060274): the window
   mode (windowed, borderless fullscreen, fullscreen), the resolution, which the game is drawn
   at, widescreen, the draw distances and the fog (pc_video.h). Left and right change a row;
   Apply switches the window, then asks whether to keep it: Accept keeps it and saves
   Settings.ini, Back or KEEP_FRAMES without an answer go back to the old settings (a fullscreen
   mode the monitor can't show leaves nothing to click on). Changes that leave the window as it
   is (the draw distances, the fog) are kept and saved straight away. Drawn like the controls
   screen above. */
#include "pc_video.h"

enum {
	GFX_MODE,
	GFX_RESOLUTION,
	GFX_WIDESCREEN,
	GFX_DRAW_DISTANCE,
	GFX_OBJECT_DISTANCE,
	GFX_FOG,
	GFX_APPLY,
	GFX_BACK,
	GFX_ROWS
};

#define KEEP_FRAMES (15 * 60)

static const char *const s_modeLabels[PC_VIDEO_MODES] = { "WINDOWED", "BORDERLESS", "FULLSCREEN" };
static const char *const s_fogLabels[PC_FOG_MODES] = { "NORMAL", "LIGHT", "OFF" };

/* the distance steps, in percent (Settings.ini may hold others in between); the last, the entire
   level, only for the draw distance */
static const int s_distances[] = { 100, 200, 300, 400, PC_DISTANCE_FULL };

#define DRAW_STEPS   5
#define OBJECT_STEPS 4

static int StepDistance(int cur, int d, int n) {
	int i;

	if (d > 0) {
		for (i = 0; i < n; i++)
			if (s_distances[i] > cur)
				return s_distances[i];

		return s_distances[0];
	}

	for (i = n - 1; i >= 0; i--)
		if (s_distances[i] < cur)
			return s_distances[i];

	return s_distances[n - 1];
}

/* the menu font has letters and digits only */
static void DistanceLabel(char *buf, int pct) {
	if (pct == PC_DISTANCE_FULL)
		sprintf(buf, "%s %s", "ENTIRE", "LEVEL");
	else if (pct == 100)
		sprintf(buf, "%s %s", "NORMAL", "");
	else if (pct % 100 == 0)
		sprintf(buf, (char *)"%dX", pct / 100);
	else
		sprintf(buf, (char *)"%d PERCENT", pct);
}

/* the resolution a mode uses */
static int *ResOf(PcVideoSettings *s, int mode) {
	return mode == PC_VIDEO_WINDOWED ? &s->win_w : &s->full_w;
}

/* step a mode's resolution through its list (borderless has only the desktop's) */
static void StepResolution(PcVideoSettings *s, int d) {
	int n = PC_VideoResolutionCount(s->mode);
	int *r = ResOf(s, s->mode);
	int i, at = -1;

	if (s->mode == PC_VIDEO_BORDERLESS || n == 0)
		return;

	for (i = 0; i < n; i++) {
		int w, h;

		PC_VideoResolution(s->mode, i, &w, &h);

		if (w == r[0] && h == r[1])
			at = i;
	}

	at = at < 0 ? (d > 0 ? 0 : n - 1) : (at + d + n) % n;
	PC_VideoResolution(s->mode, at, &r[0], &r[1]);
}

/* the same window: nothing to confirm */
static int SameWindow(const PcVideoSettings *a, const PcVideoSettings *b) {
	return a->mode == b->mode && a->win_w == b->win_w && a->win_h == b->win_h &&
	       a->full_w == b->full_w && a->full_h == b->full_h && a->widescreen == b->widescreen;
}

static int SameSettings(const PcVideoSettings *a, const PcVideoSettings *b) {
	return SameWindow(a, b) && a->draw_distance == b->draw_distance &&
	       a->object_distance == b->object_distance && a->fog == b->fog;
}

extern "C" void PC_GraphicsOptions(int a) {
	PcVideoSettings cur, edit, old;
	int sel = 0, keep = 0, i;
	int base_scale = Mess_GetFontScale();
	int small_scale = base_scale * 3 / 4;
	(void)a;

	PC_VideoGetSettings(&cur);

	/* fullscreen without a resolution of its own is at the desktop's */
	if (cur.full_w <= 0 && PC_VideoResolutionCount(PC_VIDEO_BORDERLESS))
		PC_VideoResolution(PC_VIDEO_BORDERLESS, 0, &cur.full_w, &cur.full_h);

	edit = cur;
	old = cur;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));
	DrawSync(0);

	s_saved = (ulong *)malloc(s_area.w * s_area.h * 2);

	PC_ScreenRead(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);

	pc_hires_before_draw = RestoreArea;

	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);

	DemoTimer = 1600;

	for (;;) {
		SControl *p = &Pad[0];
		char buf[64];

		Front_UpdateMusicLoop();
		FrontLoader_Continue();
		Pad_Update();
		Front_CheckSoftReset();

		if (DAT_800d1cb8)
			break;

		PollHost();
		HiRes_ClearOTag();
		ResetOT();

		/* ---- input */
		if (keep) {
			if (p->MenuAccept.Triggered) {
				keep = 0;
				cur = edit;
				PC_VideoSave();
				SFX_PlayX(145, 8192, 0);
			} else if (p->MenuBack.Triggered || --keep == 0) {
				keep = 0;
				PC_VideoApply(&old);

				edit = old;
				SFX_PlayX(147, 8192, 0);
			}
		} else {
			int left = p->MenuLeft.Triggered, right = p->MenuRight.Triggered;
			int d = right - left;

			if (Repeat(&p->MenuUp)) {
				sel = sel ? sel - 1 : GFX_ROWS - 1;
				SFX_PlayX(144, 8192, 0);
			}

			if (Repeat(&p->MenuDown)) {
				sel = sel == GFX_ROWS - 1 ? 0 : sel + 1;
				SFX_PlayX(144, 8192, 0);
			}

			if (p->MenuAccept.Triggered && sel < GFX_APPLY)
				d = 1;

			if (d && sel == GFX_MODE) {
				edit.mode = (edit.mode + d + PC_VIDEO_MODES) % PC_VIDEO_MODES;
				SFX_PlayX(144, 8192, 0);
			} else if (d && sel == GFX_RESOLUTION) {
				if (edit.mode == PC_VIDEO_BORDERLESS) {
					SFX_PlayX(147, 8192, 0);
				} else {
					StepResolution(&edit, d);
					SFX_PlayX(144, 8192, 0);
				}
			} else if (d && sel == GFX_WIDESCREEN) {
				edit.widescreen ^= 1;
				SFX_PlayX(144, 8192, 0);
			} else if (d && sel == GFX_DRAW_DISTANCE) {
				edit.draw_distance = StepDistance(edit.draw_distance, d, DRAW_STEPS);
				SFX_PlayX(144, 8192, 0);
			} else if (d && sel == GFX_OBJECT_DISTANCE) {
				edit.object_distance = StepDistance(edit.object_distance, d, OBJECT_STEPS);
				SFX_PlayX(144, 8192, 0);
			} else if (d && sel == GFX_FOG) {
				edit.fog = (edit.fog + d + PC_FOG_MODES) % PC_FOG_MODES;
				SFX_PlayX(144, 8192, 0);
			}

			if (p->MenuAccept.Triggered && sel == GFX_APPLY) {
				if (SameSettings(&edit, &cur)) {
					SFX_PlayX(147, 8192, 0);
				} else if (SameWindow(&edit, &cur)) {
					PC_VideoApply(&edit);

					cur = edit;
					PC_VideoSave();
					SFX_PlayX(145, 8192, 0);
				} else {
					old = cur;
					PC_VideoApply(&edit);

					keep = KEEP_FRAMES;
					SFX_PlayX(145, 8192, 0);
				}
			}

			if ((p->MenuAccept.Triggered && sel == GFX_BACK) || p->MenuBack.Triggered) {
				SFX_PlayX(147, 8192, 0);
				break;
			}
		}

		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		/* ---- draw */
		Mess_SetFontScale(small_scale);

		for (i = 0; i < GFX_ROWS; i++) {
			int y = ROW_TOP + (i + (i >= GFX_APPLY)) * ROW_HEIGHT + 7;
			int on = i == sel && !keep;

			switch (i) {
				case GFX_MODE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "DISPLAY MODE");
					Text(SLOT_X1 - 60, y, 0, s_modeLabels[edit.mode]);
					break;
				case GFX_RESOLUTION: {
					int w, h;

					if (edit.mode == PC_VIDEO_BORDERLESS) {
						w = h = 0;

						if (PC_VideoResolutionCount(PC_VIDEO_BORDERLESS))
							PC_VideoResolution(PC_VIDEO_BORDERLESS, 0, &w, &h);
					} else {
						w = ResOf(&edit, edit.mode)[0];
						h = ResOf(&edit, edit.mode)[1];
					}

					SetColour(on, edit.mode == PC_VIDEO_BORDERLESS);
					Text(NAME_X, y, 1, "RESOLUTION");
					sprintf(buf, (char *)"%d X %d", w, h);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				}
				case GFX_WIDESCREEN:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "WIDESCREEN");
					Text(SLOT_X1 - 60, y, 0, edit.widescreen ? "ON" : "OFF");
					break;
				case GFX_DRAW_DISTANCE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "DRAW DISTANCE");
					DistanceLabel(buf, edit.draw_distance);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				case GFX_OBJECT_DISTANCE:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "OBJECT DISTANCE");
					DistanceLabel(buf, edit.object_distance);
					Text(SLOT_X1 - 60, y, 0, buf);
					break;
				case GFX_FOG:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "FOG");
					Text(SLOT_X1 - 60, y, 0, s_fogLabels[edit.fog]);
					break;
				case GFX_APPLY:
					SetColour(on, SameSettings(&edit, &cur));
					Text(NAME_X, y, 1, "APPLY");
					break;
				default:
					SetColour(on, 0);
					Text(NAME_X, y, 1, "BACK TO OPTIONS");
					break;
			}
		}

		if (keep) {
			Mess_SetRGB(182, 132, 36, 0);
			Text(NAME_X, ROW_TOP + (GFX_ROWS + 2) * ROW_HEIGHT + 7, 1, "KEEP THESE SETTINGS");
			sprintf(buf, (char *)"ACCEPT TO KEEP   BACK TO UNDO   %d", (keep + 59) / 60);
		} else if (sel == GFX_MODE || (sel == GFX_RESOLUTION && edit.mode != PC_VIDEO_BORDERLESS)) {
			sprintf(buf, "%s %s", "LEFT OR RIGHT TO CHANGE", "");
		} else if (sel == GFX_RESOLUTION) {
			sprintf(buf, "%s %s", "BORDERLESS USES THE", "DESKTOP RESOLUTION");
		} else if (sel == GFX_WIDESCREEN) {
			sprintf(buf, "%s %s", "MORE OF THE WORLD AT THE", "SIDES OF WIDE SCREENS");
		} else if (sel == GFX_DRAW_DISTANCE) {
			sprintf(buf, "%s %s", "HOW FAR AWAY THE LEVEL", "AND ITS FOG ARE DRAWN");
		} else if (sel == GFX_OBJECT_DISTANCE) {
			sprintf(buf, "%s %s", "HOW FAR OBJECTS STAY", "DETAILED AND VISIBLE");
		} else if (sel == GFX_FOG) {
			sprintf(buf, "%s %s", "LIGHT FOGS ONLY THE", "FARTHEST PART");
		} else if (sel == GFX_APPLY && !SameSettings(&edit, &cur)) {
			sprintf(buf, "%s %s", "ACCEPT TO SWITCH", "");
		} else {
			buf[0] = 0;
		}

		Mess_SetRGB(122, 122, 122, 0);
		Text(370, HINT_Y, 0, buf);
		Mess_SetFontScale(base_scale);
		HiRes_DrawOTag();
	}

	/* a change not confirmed yet is undone */
	if (keep)
		PC_VideoApply(&old);

	pc_hires_before_draw = 0;

	PC_ScreenBlit(s_area.x, s_area.y, s_area.w, s_area.h, s_saved);
	DrawSync(0);
	free(s_saved);
	Pad_ClearTriggers(&Pad[0]);
	Pad_ClearTriggers(&Pad[1]);
}
#else
int Front_ControllerOptions(int a) {
	int pl = DAT_800d1d28;
	SLabels labels;
	int disabled[8];
	int xs[8];
	int ys[8];
	char vib[32];
	char ak[32];
	int r;

	FrontLoader_LoadSet(13);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("opt_ctr.bmp"));

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
	labels.Name[0] = Front_GetPadFunctionName(DAT_800bf260[pl].Function[0]);
	labels.Name[1] = Front_GetPadFunctionName(DAT_800bf260[pl].Function[1]);
	labels.Name[2] = Front_GetPadFunctionName(DAT_800bf260[pl].Function[2]);
	labels.Name[3] = Front_GetPadFunctionName(DAT_800bf260[pl].Function[3]);

	sprintf(vib, "%s %s", GameText.pVibration,
	        GPreferences.VibrationEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);

	labels.Name[4] = vib;

	sprintf(ak, "%s %s", GameText.pAutoKick,
	        GPreferences.AutoKickEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);

	labels.Name[5] = ak;
	DAT_800bf280[pl].Function[0] = DAT_800bf260[pl].Function[0];
	DAT_800bf280[pl].Function[2] = DAT_800bf260[pl].Function[2];
	DAT_800bf280[pl].Function[1] = DAT_800bf260[pl].Function[1];
	DAT_800bf280[pl].Function[3] = DAT_800bf260[pl].Function[3];
	r = Front_RunMenu(labels.Name, disabled, 8, 1296, &a, Front_ControllerOptionsHandler, xs, ys,
	                  0);

	if (r == -1 || r == 1304) {
		FrontLoader_LoadSet(11);
		FrontLoader_Defragment();
		DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

		r = -1;
	}

	return r;
}
#endif
