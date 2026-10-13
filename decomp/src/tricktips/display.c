/* TrickTips_Display__Fv (0x80072164) */
#include "names.h"

typedef struct {
	char pad[0x90];
	const char *s0;
	const char *s1;
	const char *s2;
	const char *s3;
	unsigned cnt;
} TT;

extern TT TrickTipMonitor;

void Mess_SetTextJustify(int);
void Mess_SetRGB(unsigned char, unsigned char, unsigned char, int);
void Mess_SetScale(int);
int Mess_GetScale(void);
int Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

void TrickTips_Display(void) {
	unsigned short y = 20;
	unsigned short t;

	if (TrickTipMonitor.s0) {
		t = 200 - TrickTipMonitor.cnt;

		if (t < 10)
			y = y - (10 - t) * 40;

		if (TrickTipMonitor.cnt < 10)
			y = y - (10 - TrickTipMonitor.cnt) * 40;

		Mess_SetTextJustify(1);
		Mess_SetRGB(128, 128, 0, 0);
		Mess_SetScale(220);
		Mess_DrawPanelText(y, 100, TrickTipMonitor.s0, 0, 0, 256);

		if (TrickTipMonitor.s1)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 + 100, TrickTipMonitor.s1, 0, 0, 256);

		if (TrickTipMonitor.s2)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 2 + 100, TrickTipMonitor.s2, 0, 0, 256);

		if (TrickTipMonitor.s3)
			Mess_DrawPanelText(y, Mess_GetScale() / 11 * 3 + 100, TrickTipMonitor.s3, 0, 0, 256);

		if (--TrickTipMonitor.cnt == 0) {
			TrickTipMonitor.s3 = 0;
			TrickTipMonitor.s2 = 0;
			TrickTipMonitor.s1 = 0;
			TrickTipMonitor.s0 = 0;
		}
	}
}
