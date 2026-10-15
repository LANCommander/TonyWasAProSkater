/* FUN_8006d0c8 (0x8006d0c8) */
/* Panel: the current combo/score readout (PANEL.CPP). Sums the pending trick scores, works out
   the multiplier, and draws the score and x-multiplier text. */
#include "game/bruce.h"
#include "game/mess.h"
#include "game/panel.h"
#include "game/psyq.h"

extern int GCareer_9b8[];
extern int GVInfo1[];

unsigned LastHoldingScore;

void GraffitiColor(CBruce *);
int Mess_DrawPanelText(int, int, const char *, int, unsigned, int);

extern "C" void FUN_8006d0c8(CBruce *o, int x0, int y0) {
	char b1[16];
	char b2[16];
	unsigned s2;
	int n2, s3, x, y, s5, s4, n, k, i, v, r;
	int *p;

	s2 = 0;
	s3 = 0;
	x = x0 & 0xffff;
	y = y0 & 0xffff;
	s5 = GVInfo1[0] != 2;

	if (GCareer_9b8[0]) {
		n = o->mPanel.mTQStack;

		if (n > 0) {
			k = o->mNum180Spins;
			n2 = n * 2;

			if (k < 3)
				v = n2 + k;
			else
				v = n2 + (k * 2 - 2);

			s3 = v;

			if (o->mPhysicsState == 2 && k == 1)
				s3 -= 1;

			for (i = 0; i < o->mPanel.mTQStack; i++)
				s2 += o->mPanel.mTQ[i].point_value;
		}

		if (o->mPanel.mLandedTotal)
			s2 = o->mPanel.mLandedTotal;

		if (s2 || o->mPanel.mTrickState == 5) {
			sprintf(b1, "%ld", (long)(o->mPanel.mTrickState == 5 ? LastHoldingScore : s2));
			Mess_SetRGB(128, 128, 128, 0);
			GraffitiColor(o);

			s4 = s3 < 3;

			if (!s4)
				Mess_SetTextJustify(2);
			else
				Mess_SetTextJustify(0);

			Mess_SetScale(256);

			if (s5) {
				o->mPanel.mFallingTextEffect.bind(o->mPanel.mTrickState == 5);
				Mess_DrawPanelText(x, y, b1, 0, 0, 256);
				o->mPanel.mFallingTextEffect.bind(0);
			}

			if (o->mPanel.m4F4 > 0)
				o->mPanel.m4F4 = o->mPanel.m4F4 - 1;

			if (!s4) {
				if (s3 & 1)
					sprintf(b2, " x%d.5", s3 >> 1);
				else
					sprintf(b2, " x%d", s3 >> 1);

				Mess_SetRGB(128, 128, 128, 0);
				GraffitiColor(o);
				Mess_SetTextJustify(1);
				Mess_SetScale(256);

				if (s5)
					Mess_DrawPanelText(x, y, b2, 0, 0, 256);
			}

			if (s2)
				LastHoldingScore = (s2 * s3) >> 1;
		}

		if (o->mPanel.mTQStack) {
			int kk = o->mNum180Spins;

			if (o->mPanel.old_180spins < kk) {
				if (o->mPhysicsState != 2 || kk >= 2) {
					o->mPanel.TrickMessage();

					o->mPanel.old_180spins = o->mNum180Spins;
				}
			}
		}
	}
}
