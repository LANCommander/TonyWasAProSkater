/* TrickMessageRefresh__10CPanelData (0x8006e5ac) */
/* C++ (PANEL.CPP). Demo twin CPanelData::TrickMessageRefresh: THPS1 drops the per-trick spin
   display. Rebuilds the up-to-three trick-name lines: an optional spin prefix ("%d "), the
   stance name and the first trick, then each further trick appended to the current line,
   wrapping when the line gets wider than the viewport allows. FUN_8008b900 = sprintf,
   Mess_TextWidth__FPCc = text width, FUN_8006e534 = "SWITCH " or "" prefix. */
struct Message;

class CBruce {
  public:
	char pad0[0x32c];
	int m32C; /* 0x32c spin (half turns) */
	char pad330[0x734 - 0x330];
	int m734; /* 0x734 */
	char pad738[0x92c - 0x738];
	unsigned short mPlayer; /* 0x92c */
};

class CTrick {
  public:
	char *name;       /* 0x00 */
	int point_value;  /* 0x04 */
	int depreciate;   /* 0x08 */
	int countdown;    /* 0x0c */
	Message *message; /* 0x10 */
};

class CPanelData {
  public:
	CBruce *mpBruce; /* 0x000 */
	char pad4[0x1f8 - 0x4];
	CTrick mTQ[20];        /* 0x1f8 */
	int mTQStack;          /* 0x388 */
	char mTrickLine1[100]; /* 0x38c */
	char mTrickLine2[100]; /* 0x3f0 */
	char mTrickLine3[100]; /* 0x454 */
	void TrickMessageRefresh();
};

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

	mTrickLine2[0] = 0;
	mTrickLine3[0] = 0;

	if (mpBruce->m32C > 0 && (mpBruce->m734 != 2 || mpBruce->m32C >= 2))
		p = mTrickLine1 + FUN_8008b900(mTrickLine1, s_d_800d1ea8, mpBruce->m32C * 180);
	else
		p = mTrickLine1;

	currLine = 0;

	switch (ViewportMode * 2 + mpBruce->mPlayer) {
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
		mTrickLine1[0] = 0;
		return;
	}

	FUN_8008b900(p, s_s_s_800d1eac, FUN_8006e534(mpBruce), mTQ[0].name);
	Mess_SetScale(188);

	for (int i = 1; i < mTQStack; i++) {
		FUN_8008b900(tbuf, s_s_s_800d1eb4, p, mTQ[i].name);

		if (Mess_TextWidth__FPCc(tbuf) > lineWidth) {
			currLine++;

			if (currLine == 1)
				p = mTrickLine2;
			else if (currLine == 2)
				p = mTrickLine3;

			if (currLine == 3) {
				FUN_8008b900(p, s_s, p);
				return;
			}

			FUN_8008b900(p, s_s_800d1ebc, mTQ[i].name);
		} else
			FUN_8008b900(p, tbuf);
	}
}
