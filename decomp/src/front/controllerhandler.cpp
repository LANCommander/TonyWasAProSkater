/* FUN_8005eb78 (0x8005eb78) */
/* C++ (front end, THPS1-only; no demo twin). The per-frame handler of the CONTROLLER options menu:
   FUN_8005f6bc passes it to the generic menu runner FUN_8005e0e0, which calls it with the
   VerticalMenu every frame (and, seeing this handler, binds the menu input to player
   DAT_800d1d28's pad). Rows: 0-3 = TRIANGLE/SQUARE/CIRCLE/X (each row shows the function mapped
   to that button), 4 = VIBRATION, 5 = AUTOKICK, 6 = RESET TO DEFAULT, 7 = BACK.
     - Player 2 with no second pad: fake a triangle press on pad 2 so the menu backs out.
     - Up/down/X/start: the row that was just changed (DAT_800d2350) keeps its new function and any
       other button that had the same function takes over the changed button's old one, so the
       mapping stays a permutation; the snapshot DAT_800bf280 is updated, the other rows' labels
       refreshed and the mapping committed to the options (FUN_8005eb28).
     - Left/right on a button row cycles its function 0..3; on VIBRATION/AUTOKICK sets it
       (left = on) and rebuilds the "%s %s" label (AUTOKICK also changes button labels).
     - X/start on RESET TO DEFAULT restores the identity mapping and turns both options on.
   The four left/right button-row blocks index by the stored selection (`Function[sel]`,
   `DAT_800d2350 = sel`), which keeps retail's unfolded (&map + 4k) + pl*16 address and the s2
   copy of the selection. The row address is built column-first (`b = &map[0].Function[sel];
   f = b + pl * 4;`), which puts the DAT_800bf260 `lui` ahead of `sll pl,4`. `f` is a block-local
   (set once per pseudo), so alias.c knows its base is DAT_800bf260 and lets the reload of *f move
   above the DAT_800d2350 byte store; one function-wide `f` set four times loses its base value. */
#include "game/career.h"
#include "game/pad.h"
#include "game/front.h"
#include "game/shellutils.h"

extern SControl Pad[2]; /* Pad */
extern SOptions GPreferences;
extern SPadMap DAT_800bf260[2]; /* edited mapping */
extern SPadMap DAT_800bf280[2]; /* mapping before the last change */
extern LocalText GameText;
extern char s_s_s[]; /* "%s %s" */

int DAT_800d1d28;   /* player being configured */
uchar DAT_800d2350; /* row last changed with left/right */

int SFX_PlayX(unsigned, short, int);

extern "C" {
	int FUN_8008b900(char *, char *, ...);
	char *FUN_8005ea74(int);
	void FUN_8005eb28(int);
	void FUN_8005eb78(VerticalMenu *);
}

void FUN_8005eb78(VerticalMenu *menu) {
	int pl = DAT_800d1d28;
	char buf[32];
	uchar left;
	int sel;

	if (pl == 1 && !Pad[1].Type) {
		Pad[1].MenuBack.Triggered = 1;
		return;
	}

	Pad_ClearTriggers(&Pad[1 - pl]);

	if (Pad[pl].MenuStart.Triggered || Pad[pl].MenuAccept.Triggered ||
	    Pad[pl].MenuUp.Triggered || Pad[pl].MenuDown.Triggered) {
		switch (DAT_800d2350) {
			case 0:
				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[0];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[0];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[0])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[0];

				break;
			case 1:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[1];

				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[1];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[1])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[1];

				break;
			case 2:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[2];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[2];

				if (DAT_800bf260[pl].Function[3] == DAT_800bf260[pl].Function[2])
					DAT_800bf260[pl].Function[3] = DAT_800bf280[pl].Function[2];

				break;
			case 3:
				if (DAT_800bf260[pl].Function[0] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[0] = DAT_800bf280[pl].Function[3];

				if (DAT_800bf260[pl].Function[2] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[2] = DAT_800bf280[pl].Function[3];

				if (DAT_800bf260[pl].Function[1] == DAT_800bf260[pl].Function[3])
					DAT_800bf260[pl].Function[1] = DAT_800bf280[pl].Function[3];

				break;
		}

		DAT_800bf280[pl].Function[0] = DAT_800bf260[pl].Function[0];
		DAT_800bf280[pl].Function[1] = DAT_800bf260[pl].Function[1];
		DAT_800bf280[pl].Function[2] = DAT_800bf260[pl].Function[2];
		DAT_800bf280[pl].Function[3] = DAT_800bf260[pl].Function[3];

		if (menu->getSelected() != 0)
			menu->setString(0, FUN_8005ea74(DAT_800bf260[pl].Function[0]));

		if (menu->getSelected() != 1)
			menu->setString(1, FUN_8005ea74(DAT_800bf260[pl].Function[1]));

		if (menu->getSelected() != 2)
			menu->setString(2, FUN_8005ea74(DAT_800bf260[pl].Function[2]));

		if (menu->getSelected() != 3)
			menu->setString(3, FUN_8005ea74(DAT_800bf260[pl].Function[3]));

		FUN_8005eb28(pl);
	}

	if (Pad[pl].MenuLeft.Triggered || Pad[pl].MenuRight.Triggered) {
		left = Pad[pl].MenuLeft.Triggered;
		Pad_ClearTriggers(&Pad[0]);
		Pad_ClearTriggers(&Pad[1]);

		if ((sel = menu->getSelected()) == 0) {
			SFX_PlayX(144, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, FUN_8005ea74(*f));
		}

		if ((sel = menu->getSelected()) == 1) {
			SFX_PlayX(144, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, FUN_8005ea74(*f));
		}

		if ((sel = menu->getSelected()) == 2) {
			SFX_PlayX(144, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, FUN_8005ea74(*f));
		}

		if ((sel = menu->getSelected()) == 3) {
			SFX_PlayX(144, 8192, 0);
			int *b = &DAT_800bf260[0].Function[sel];
			int *f = b + pl * 4;

			*f = *f + (left ? -1 : 1);

			if ((signed char)*f < 0)
				*f = 3;

			if (*f >= 4)
				*f = 0;

			DAT_800d2350 = sel;
			menu->setString(sel, FUN_8005ea74(*f));
		}

		if (menu->getSelected() == 4) {
			if (left != GPreferences.VibrationEnabled[pl])
				SFX_PlayX(144, 8192, 0);

			GPreferences.VibrationEnabled[pl] = left != 0;
			FUN_8008b900(buf, s_s_s, GameText.pVibration,
			             GPreferences.VibrationEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);
			menu->setString(4, buf);
		}

		if (menu->getSelected() == 5) {
			if (left != GPreferences.AutoKickEnabled[pl])
				SFX_PlayX(144, 8192, 0);

			GPreferences.AutoKickEnabled[pl] = left != 0;
			FUN_8008b900(buf, s_s_s, GameText.pAutoKick,
			             GPreferences.AutoKickEnabled[pl] ? GameText.pBooleanOn : GameText.pBooleanOff);
			menu->setString(5, buf);
			menu->setString(0, FUN_8005ea74(DAT_800bf260[pl].Function[0]));
			menu->setString(1, FUN_8005ea74(DAT_800bf260[pl].Function[1]));
			menu->setString(2, FUN_8005ea74(DAT_800bf260[pl].Function[2]));
			menu->setString(3, FUN_8005ea74(DAT_800bf260[pl].Function[3]));
		}
	}

	if (Pad[pl].MenuStart.Triggered || Pad[pl].MenuAccept.Triggered) {
		if (menu->getSelected() != 7) {
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);
		}

		if (menu->getSelected() == 6) {
			DAT_800bf260[pl].Function[0] = 0;
			DAT_800bf260[pl].Function[1] = 1;
			DAT_800bf260[pl].Function[2] = 2;
			DAT_800bf260[pl].Function[3] = 3;
			FUN_8005eb28(pl);

			GPreferences.VibrationEnabled[pl] = 1;
			FUN_8008b900(buf, s_s_s, GameText.pVibration, GameText.pBooleanOn);
			menu->setString(4, buf);

			GPreferences.AutoKickEnabled[pl] = 1;
			FUN_8008b900(buf, s_s_s, GameText.pAutoKick, GameText.pBooleanOn);
			menu->setString(5, buf);
			menu->setString(0, FUN_8005ea74(DAT_800bf260[pl].Function[0]));
			menu->setString(1, FUN_8005ea74(DAT_800bf260[pl].Function[1]));
			menu->setString(2, FUN_8005ea74(DAT_800bf260[pl].Function[2]));
			menu->setString(3, FUN_8005ea74(DAT_800bf260[pl].Function[3]));
		}

		if (menu->getSelected() != 4 && menu->getSelected() != 5)
			SFX_PlayX(145, 8192, 0);
	}
}
