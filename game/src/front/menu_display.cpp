/* Display__5CMenu (0x8001696c) flags=-fstructure-noalias */
/* C++ (FRONT.CPP). Demo twin (0.97); members, locals and blocks from its MAIN.SYM. Draws each
   visible menu line at its current scale: the selected line in its highlight colour, the others
   fading up from half brightness; then, for the cursor line of a left-justified menu, a pair of
   red arrowheads either side of the text. Needs -fstructure-noalias: retail loads pDoubleBuffer
   (the OT) above the right arrow's halfword stores. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/front.h"
#include "game/m3d.h"
#include "game/mess.h"

extern uchar *pPoly;
extern uchar *PolyBufferEnd;
extern SDoubleBuffer *pDoubleBuffer;

void Mess_SetScale(int);
void Mess_SetRGB(uchar, uchar, uchar, int);

void CMenu::Display() {
	int y;

	Mess_SetTextJustify(mJustification);

	y = mY;

	{
		int i;

		for (i = 0; i < mNumLines; i++) {
			y += mEntry[i].Clearance;

			if (mEntry[i].Visible) {
				if (mEntry[i].CurrentScale > 0) {
					Mess_SetScale(mEntry[i].CurrentScale);

					if (i == mLine) {
						Mess_SetRGB(mEntry[i].HiR, mEntry[i].HiG, mEntry[i].HiB, 0);

						mEntry[i].FadeUpCounter = 0;
					} else {
						if (mEntry[i].Enabled)
							mEntry[i].FadeUpCounter = 128;
						else
							mEntry[i].FadeUpCounter = 64;

						Mess_SetRGB(mEntry[i].LowR * mEntry[i].FadeUpCounter >> 7,
						            mEntry[i].LowG * mEntry[i].FadeUpCounter >> 7,
						            mEntry[i].LowB * mEntry[i].FadeUpCounter >> 7, 0);

						mEntry[i].FadeUpCounter += 60;

						if (mEntry[i].FadeUpCounter > 128)
							mEntry[i].FadeUpCounter = 128;
					}

					if (mEntry[i].LowScale < mEntry[i].CurrentScale)
						Mess_SetSort(0);
					else
						Mess_SetSort(1);

					{
						int length = Mess_DrawText(mX, y, mEntry[i].pString, 0, 0, 0, 0);

						if (mHasCursor && i == mCursorLine && !mJustification) {
							int off = length * mEntry[i].CurrentScale / 512 +
							          mEntry[i].CurrentScale * 14 / 256;
							POLY_F3 *leftpoly = (POLY_F3 *)pPoly;
							POLY_F3 *rightpoly = leftpoly + 1;
							uchar *newppoly = (uchar *)(rightpoly + 1);

							if (newppoly <= PolyBufferEnd) {
								pPoly = newppoly;
								setPolyF3(leftpoly);
								setPolyF3(rightpoly);
								setRGB0(leftpoly, 255, 0, 0);

								leftpoly->x0 = mX - off;
								leftpoly->y0 = y;
								leftpoly->x1 = leftpoly->x0 - 20;
								leftpoly->y1 = y - 6;
								leftpoly->x2 = leftpoly->x0 - 20;
								leftpoly->y2 = y + 6;
								setRGB0(rightpoly, 255, 0, 0);

								rightpoly->x0 = mX + off;
								rightpoly->y0 = y;
								rightpoly->x1 = rightpoly->x0 + 20;
								rightpoly->y1 = y - 6;
								rightpoly->x2 = rightpoly->x0 + 20;
								rightpoly->y2 = y + 6;

								{
									P_TAG *ot = &pDoubleBuffer->OT2D;

									addPrim(ot, leftpoly);
									addPrim(ot, rightpoly);
								}
							}
						}
					}
				}

				y += mLineSep;
			}
		}
	}
}
