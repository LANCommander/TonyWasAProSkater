/* FUN_80025440 (0x80025440) */
#include "names.h"

extern int GamePaused;
extern int Xblanks;
extern int DAT_800a7214[];
extern char DAT_800d2580[], s_d_FPS[];

int DAT_800d1750, LastXblanks, Cheat_SlowMo, TimeScale, XFramesShifted, XFrames, TimeScaleSquared,
    Cheat_FPS, DAT_800d2230, GameFrozen;

int FUN_8008b900(char *, char *, int);
void Font_String(unsigned short, unsigned short, char *, unsigned char, unsigned char,
                 unsigned char, unsigned char);
void Font_Display(void);

int FUN_80025440(void) {
	int a1, i, s1, t;

	if (GamePaused == 0) {
		t = DAT_800d1750 + 1;
		DAT_800a7214[DAT_800d1750] = Xblanks - LastXblanks;
		DAT_800d1750 = t;

		if (t == 3)
			DAT_800d1750 = 0;
	}

	a1 = 0;
	i = 0;
	LastXblanks = Xblanks;

	do {
		a1 += DAT_800a7214[i];
		i++;
	} while (i < 3);

	s1 = 180 / a1;

	if (a1 >= 16)
		a1 = 15;

	if (GamePaused == 0) {
		TimeScale = (a1 << 8) / 6;

		if (Cheat_SlowMo)
			TimeScale = TimeScale / 4;

		XFramesShifted = XFramesShifted + TimeScale * 2;
		XFrames = XFramesShifted >> 8;
		TimeScaleSquared = (TimeScale * TimeScale) >> 8;
	}

	if (Cheat_FPS) {
		DAT_800d2230 = (DAT_800d2230 + 10) & 127;
		FUN_8008b900(DAT_800d2580, s_d_FPS, s1);

		if (GameFrozen == 0) {
			if (s1 >= 25)
				Font_String(450, 210, DAT_800d2580, 50, 100, 50, 1);
			else if (s1 >= 22)
				Font_String(450, 210, DAT_800d2580, 100, 100, 50, 1);
			else
				Font_String(450, 210, DAT_800d2580, (DAT_800d2230 + 50) & 255, 50, 50, 1);

			Font_Display();
		}
	}
}
