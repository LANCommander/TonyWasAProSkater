/* TrickMessageRefresh__10CPanelData (0x8006e5ac) */
/* C++ (PANEL.CPP). Demo twin CPanelData::TrickMessageRefresh: THPS1 drops the per-trick spin
   display. Rebuilds the up-to-three trick-name lines: an optional spin prefix ("%d "), the
   stance name and the first trick, then each further trick appended to the current line,
   wrapping when the line gets wider than the viewport allows. sprintf = sprintf,
   Mess_TextWidth__FPCc = text width, Panel_SwitchText = "SWITCH " or "" prefix. */
#include "game/panel.h"
#include "game/bruce.h"
#include "game/mess.h"
#include "game/psyq.h"

extern int ViewportMode;

void Mess_SetScale(int);

void CPanelData::TrickMessageRefresh() {
	char *p;
	char tbuf[96];
	int lineWidth;
	int currLine;

	mTrickName2[0] = 0;
	mTrickName3[0] = 0;

	if (mpBruce->mNum180Spins > 0 && (mpBruce->mPhysicsState != 2 || mpBruce->mNum180Spins >= 2))
		p = mTrickName + sprintf(mTrickName, "%d ", mpBruce->mNum180Spins * 180);
	else
		p = mTrickName;

	currLine = 0;

	switch (ViewportMode * 2 + mpBruce->mPanel.mPlayerNum) {
		case 2:
		case 3:
		case 6:
		case 7:
			lineWidth = 240;
			break;
		default:
			lineWidth = 480;
			break;
	}

	if (!mTQStack) {
		mTrickName[0] = 0;
		return;
	}

	sprintf(p, "%s%s", Panel_SwitchText(mpBruce), mTQ[0].name);
	Mess_SetScale(188);

	for (int i = 1; i < mTQStack; i++) {
		sprintf(tbuf, "%s + %s", p, mTQ[i].name);

		if (Mess_TextWidth(tbuf) > lineWidth) {
			currLine++;

			if (currLine == 1)
				p = mTrickName2;
			else if (currLine == 2)
				p = mTrickName3;

			if (currLine == 3) {
				sprintf(p, "%s + ^...^", p);
				return;
			}

			sprintf(p, "+ %s", mTQ[i].name);
		} else
			sprintf(p, tbuf);
	}
}
