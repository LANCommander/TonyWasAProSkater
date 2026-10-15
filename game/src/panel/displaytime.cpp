/* DisplayTime__FP6CBruceUsUs (0x8006d39c) */
/* C++ (PANEL.CPP). Fuzzy demo twin (0.31 by opcode, the same shape with the Mess_ text calls);
   locals from its MAIN.SYM. Draws a skater's run clock as m:ss, flashing in the last seconds, and
   ticks the urgent beep (faster as time runs out) for the first player. The two SVideoInfo blocks
   are one extern array: an unsized extern gives the split lui/addiu that keeps the if/else (a
   single `la` lets jump.c turn it into `VInfo = B; if (...) VInfo = A`). */
#include "game/bruce.h"
#include "game/mess.h"
#include "game/psyq.h"
#include "game/video.h"

extern int GGame;
extern CBruce *MechList;
extern CBruce *GSkater;
extern SVideoInfo GVInfo1[];
extern int XFrames;
extern uint Xblanks;
extern int GStartTime;
extern int GSkaterRegion;

int Urgent;

int SFX_PlayX(unsigned int, short, int);
void Mess_SetRGB(uchar, uchar, uchar, int);
void Mess_SetTextJustify(uchar);
void Mess_SetScale(int);
int Mess_DrawPanelText(int, int, const char *, int, uint, int);

void DisplayTime(CBruce *pBruce, ushort xpos, ushort ypos) {
	char time[8];
	int ticks;
	int mins;
	int secs;

	if (GGame == 2)
		return;

	if (GGame == 7 && !pBruce->mTime)
		return;

	Mess_SetCurrentFont(GSkaterRegion);

	{
		SVideoInfo *VInfo;

		if (pBruce == MechList)
			VInfo = &GVInfo1[0];
		else
			VInfo = &GVInfo1[1];

		if (VInfo->VideoMode != 2) {
			ticks = pBruce->mTime - XFrames;
			mins = ticks / 3600;
			secs = (ticks - mins * 3600) / 60;

			if (ticks < 0) {
				secs = 0;
				mins = 0;
			}

			sprintf(time, "%ld:%02ld", (long)mins, (long)secs);
			Mess_SetRGB(128, 0, 0, 0);
			Mess_SetTextJustify(0);
			Mess_SetScale(256);

			if (GGame == 7) {
				if (mins || secs >= 4 || Xblanks % 30 >= 15)
					Mess_DrawPanelText(xpos, ypos, time, 0, 0, 256);
			} else {
				if (mins || secs >= 11 || Xblanks % 60 >= 30)
					Mess_DrawPanelText(xpos, ypos, time, 0, 0, 256);

				if (pBruce == GSkater) {
					if (ticks >= 1 && ticks < 600) {
						if (ticks < Urgent) {
							int sub;

							SFX_PlayX(22, 8192, 0);

							sub = Urgent / 10;

							if (sub < 10)
								sub = 10;

							Urgent -= sub;
						}
					} else {
						Urgent = 600;
					}
				}
			}
		}
	}

	Mess_SetCurrentFont(GStartTime);
}
