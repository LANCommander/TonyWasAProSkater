/* FUN_80061270 (0x80061270) */
#include "names.h"
#include "pc_frame.h"

int DAT_800d1d84;
int DAT_800d1d88;
short DAT_800d1d8c, DAT_800d1d8e;

void HiRes_ClearOTag(void);
void ResetOT(void);
void Sprite_draw(int, int, int, int, int, int, int);
void Mess_SetRGB(int, int, int, int);
void Mess_SetTextJustify(int);
void Mess_SetFontShadowOffset(int);
void Mess_SetFontScale(int);
void Mess_DrawText(int, int, char *, int, int, int, int);
void HiRes_DrawOTag(void);
void VSync(int);

void FUN_80061270(int a) {
	int i;

	if (a != DAT_800d1d84) {
		DAT_800d1d84 = a;

		for (i = 0; i < 2; i++) {
			HiRes_ClearOTag();
			ResetOT();
			Sprite_draw(DAT_800d1d88, DAT_800d1d8c, DAT_800d1d8e, 0, -1, -1, -1);
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(0);
			Mess_SetFontShadowOffset(4);

			if (a == 0) {
				Mess_SetFontScale(192);
				Mess_DrawText(320, 357, "checking memory card", 1, 0, 256, 0);
			} else if (a == 1) {
				Mess_SetFontScale(144);
				Mess_DrawText(320, 341, "Loading saved data from MEMORY CARD", 1, 0, 256, 0);
				Mess_DrawText(320, 373, "in MEMORY CARD slot 1", 1, 0, 256, 0);
			}

			HiRes_DrawOTag();
		}

		Mess_SetFontScale(255);
		PC_FrameWait(1);
	}
}
