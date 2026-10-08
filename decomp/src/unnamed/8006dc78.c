/* FUN_8006dc78 (0x8006dc78) */
#include "names.h"
typedef struct P {
	char pad[0x1c];
	struct P *next;
	char pad2[0x924 - 0x20];
	char sub[8];
	unsigned short sel;
	char pad3[0xa88 - 0x92e];
	char a[0x4c];
	char b[1];
} P;

extern int Cheat_PanelOn;
extern int GamePaused;
extern int ViewportMode;
extern P *MechList;

int TAPES_Display, SKATE_Display;

void DisplayTapesSkate(void);
void FUN_8001e07c(char *);
void FUN_8001e100(char *, int);
void Panel_DrawFlatShadedPoly(int, int, int, int, int, int, int, int);
void DisplayScore(P *, int, int);
void FUN_8006d0c8(P *, int, int);
void DisplayTime(P *, int, int);
void DisplayOutOfBounds(P *, int, int);
void CPanelData_DisplayTrick(char *, P *, int, int);
void Panel_ReplayInstructions(void);
void TrickTips_Display(void);

void FUN_8006dc78(void) {
	P *s2;
	char *s3;

	if (Cheat_PanelOn) {
		s2 = MechList;
		Panel_ReplayInstructions();

		if (s2) {
			s3 = (char *)s2 + 0xa88;

			do {
				FUN_8001e07c(s3);
				DisplayOutOfBounds(s2, 256, 90);
				DisplayTapesSkate();

				if (GamePaused) {
					SKATE_Display = 30;
					TAPES_Display = 30;
					return;
				}

				switch (ViewportMode * 2 + s2->sel) {
					case 0:
					case 1:
						TrickTips_Display();
						FUN_8006d0c8(s2, 256, 224);
						CPanelData_DisplayTrick(s2->sub, s2, 256, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						break;
					case 2:
						FUN_8006d0c8(s2, 384, 224);
						CPanelData_DisplayTrick(s2->sub, s2, 384, 205);
						DisplayScore(s2, 358, 45);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						break;
					case 4:
						CPanelData_DisplayTrick(s2->sub, s2, 256, 205);
						FUN_8006d0c8(s2, 256, 224);
						DisplayScore(s2, 25, 145);
						DisplayTime(s2, 452, 150);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						break;
					case 6:
						FUN_8006d0c8(s2, 128, 224);
						CPanelData_DisplayTrick(s2->sub, s2, 128, 205);
						DisplayScore(s2, 112, 150);
						DisplayTime(s2, 55, 155);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						Panel_DrawFlatShadedPoly(256, 0, 256, 120, 30, 30, 30, 1);
						break;
					case 8:
						FUN_8006d0c8(s2, 256, 224);
						CPanelData_DisplayTrick(s2->sub, s2, 256, 205);
						DisplayScore(s2, 70, 175);
						DisplayTime(s2, 133, 217);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						break;
					case 3:
						FUN_8006d0c8(s2, 128, 224);
						CPanelData_DisplayTrick(s2->sub, s2, 128, 205);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 256, 48);
						FUN_8001e100(s3, 240);
						FUN_8001e100((char *)s2 + 0xad4, 240);
						break;
					case 5:
						CPanelData_DisplayTrick(s2->sub, s2, 256, 85);
						FUN_8006d0c8(s2, 256, 104);
						DisplayScore(s2, 25, 45);
						DisplayTime(s2, 452, 50);
						FUN_8001e100(s3, 118);
						FUN_8001e100((char *)s2 + 0xad4, 118);
						break;
					case 7:
						FUN_8006d0c8(s2, 384, 104);
						CPanelData_DisplayTrick(s2->sub, s2, 384, 85);
						DisplayScore(s2, 276, 45);
						DisplayTime(s2, 457, 50);
						Panel_DrawFlatShadedPoly(0, 120, 257, 2, 30, 30, 30, 1);
						Panel_DrawFlatShadedPoly(0, 120, 257, 120, 20, 20, 20, 1);
						Panel_DrawFlatShadedPoly(256, 119, 512, 2, 30, 30, 30, 1);
						FUN_8001e100(s3, 122);
						FUN_8001e100((char *)s2 + 0xad4, 122);
						break;
					case 9:
						FUN_8006d0c8(s2, 256, 154);
						CPanelData_DisplayTrick(s2->sub, s2, 256, 130);
						DisplayScore(s2, 315, 43);
						DisplayTime(s2, 371, 85);
						FUN_8001e100(s3, 152);
						FUN_8001e100((char *)s2 + 0xad4, 152);
						break;
				}

				s2 = s2->next;
				s3 = (char *)s2 + 0xa88;
			} while (s2);
		}
	}
}
