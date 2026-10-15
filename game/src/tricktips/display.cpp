/* TrickTips_Display__Fv (0x80072164) */
#include "game/panel.h"
#include "game/mess.h"

extern CTrickTipMonitor TrickTipMonitor;

int Mess_GetScale();
int Mess_DrawPanelText(int, int, const char *, int, uint, int);

void TrickTips_Display() {
	unsigned short y = 20;
	unsigned short t;

	if (TrickTipMonitor.mTipLine1) {
		t = 200 - TrickTipMonitor.mTipTimer;

		if (t < 10)
			y = y - (10 - t) * 40;

		if (TrickTipMonitor.mTipTimer < 10)
			y = y - (10 - TrickTipMonitor.mTipTimer) * 40;

		Mess_SetTextJustify(1);
		Mess_SetRGB(128, 128, 0, 0);
		Mess_SetScale(220);
		Mess_DrawPanelText(y, 100, TrickTipMonitor.mTipLine1, 0, 0, 256);

		if (TrickTipMonitor.mTipLine2)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 + 100, TrickTipMonitor.mTipLine2, 0, 0, 256);

		if (TrickTipMonitor.mTipLine3)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 2 + 100, TrickTipMonitor.mTipLine3, 0, 0, 256);

		if (TrickTipMonitor.mTipLine4)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 3 + 100, TrickTipMonitor.mTipLine4, 0, 0, 256);

		if (--TrickTipMonitor.mTipTimer == 0) {
			TrickTipMonitor.mTipLine4 = 0;
			TrickTipMonitor.mTipLine3 = 0;
			TrickTipMonitor.mTipLine2 = 0;
			TrickTipMonitor.mTipLine1 = 0;
		}
	}
}
