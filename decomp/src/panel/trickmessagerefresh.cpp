/* TrickMessageRefresh__10CPanelData (0x8006e5ac) */
/* C++ (PANEL.CPP). Demo twin CPanelData::TrickMessageRefresh: THPS1 drops the per-trick spin
   display. Rebuilds the up-to-three trick-name lines: an optional spin prefix ("%d "), the
   stance name and the first trick, then each further trick appended to the current line,
   wrapping when the line gets wider than the viewport allows. FUN_8008b900 = sprintf,
   Mess_TextWidth__FPCc = text width, FUN_8006e534 = "SWITCH " or "" prefix. */
#include "game/panel.h"
#include "game/bruce.h"

extern int ViewportMode;
extern char s_d_800d1ea8[4]; /* "%d " */
extern char s_s_s_800d1eac[8]; /* "%s%s" */
extern char s_s_s_800d1eb4[];  /* "%s + %s" */
extern char s_s_800d1ebc[];  /* "+ %s" */
extern char s_s[8]; /* "%s + ^...^" */

extern "C" {
	int FUN_8008b900(char *, const char *, ...);
	int Mess_TextWidth__FPCc(char *);
	char *FUN_8006e534(CBruce *);
}

void Mess_SetScale(int);

void CPanelData::TrickMessageRefresh() {
	char *p;
	char tbuf[96];
	int lineWidth;
	int currLine;

	mTrickName2[0] = 0;
	mTrickName3[0] = 0;

	if (mpBruce->mNum180Spins > 0 && (mpBruce->mPhysicsState != 2 || mpBruce->mNum180Spins >= 2))
		p = mTrickName + FUN_8008b900(mTrickName, s_d_800d1ea8, mpBruce->mNum180Spins * 180);
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

	FUN_8008b900(p, s_s_s_800d1eac, FUN_8006e534(mpBruce), mTQ[0].name);
	Mess_SetScale(188);

	for (int i = 1; i < mTQStack; i++) {
		FUN_8008b900(tbuf, s_s_s_800d1eb4, p, mTQ[i].name);

		if (Mess_TextWidth__FPCc(tbuf) > lineWidth) {
			currLine++;

			if (currLine == 1)
				p = mTrickName2;
			else if (currLine == 2)
				p = mTrickName3;

			if (currLine == 3) {
				FUN_8008b900(p, s_s, p);
				return;
			}

			FUN_8008b900(p, s_s_800d1ebc, mTQ[i].name);
		} else
			FUN_8008b900(p, tbuf);
	}
}
