/* Front_MemCardHiResDisplay__Fv (0x8001206c) */
/* C++ (RFRONT.CPP), from the demo twin + its MAIN.SYM locals (i, XMid, block `load`, block `up`).
   Hi-res memory-card screen: the save/load progress screen, the background state machine, then
   one case per card state (case 20 = the save-file VerticalMenu with scroll arrows).
   Levers: the arrow text is `if (up && down) ... else sprintf(.., up ? a : b)` with `down` a
   separate block local (keeps up in a1 and XMid in s1); FUN_8008c468 returns int; the
   VerticalMenu ?: count is `n < 6 ? n : 5`; sizeof(VerticalMenu) = 820. */
typedef struct {
	char pad0[0xc];
	char *f0c;
	char *f10;
	char pad1[0x104 - 0x14];
	char *f104;
	char *f108;
	char *f10c;
	char pad2[0x128 - 0x110];
	char *f128;
	char *f12c;
	char *f130;
	char *f134;
	char pad3[0x180 - 0x138];
	char *f180;
	char pad4[0x1c8 - 0x184];
	char *f1c8;
	char *f1cc;
	char *f1d0;
	char *f1d4;
	char pad5[0x1f8 - 0x1d8];
	char *f1f8;
	char *f1fc;
	char *f200;
	char pad6[0x244 - 0x204];
	char *f244;
} S;

class CItem {
  public:
	virtual ~CItem();
	void *operator new(unsigned int) throw();
};

class VerticalMenu : public CItem {
  public:
	char pad[816];
	virtual void draw();
	VerticalMenu(int, int, int, char **, int, int *, int *, int);
	void erase();
	int getSelected();
	void setString(int, char *);
};

extern "C" {
	extern S GameText;
	extern char *GameText_180[];
	extern char *VertMenuItem[];
	extern char s_loading_video_file[];
	extern char s_loading_saved_game[];
	extern char s_Video_loaded[];
	extern char msg[];
	extern int TTime;
}

VerticalMenu *pVertMenu;
int NumVertMenuItems, VertMenuTopIndex, VertMenuTopChanged, MCState, OldMCState, MCResolution,
    GLoadAction;

extern "C" {
	void FUN_80011214(void);
	void FUN_80011570(void);
	void Mess_SetTextJustify__FUc(int);
	void FUN_80061ea0(void);
	void FUN_8006b240(void);
	void FUN_8006b26c(void);
	void FUN_8008b900(char *, char *, char *);
	int FUN_8008c468(int);
}

void Front_DirtyRectText(int, int, const char *);
void Front_PrintCentredPrompt(const char *);
void Front_PrintGoBackSelect();
void Front_RestoreBackground();
void Mess_SetScale(int);
void RGBRed();

void Front_MemCardHiResDisplay() {
	int i;
	int XMid;

	Mess_SetScale(200);

	XMid = MCResolution >> 1;

	if (MCState == 21 && GLoadAction == 2) {
		{
			char *load;

			for (i = 0; i < 2; i++) {
				FUN_8006b240();
				FUN_80061ea0();

				if (pVertMenu)
					pVertMenu->erase();

				FUN_8006b26c();
			}

			load = s_loading_saved_game;
			FUN_8008c468(0);

			for (i = 0; i < 4; i++) {
				FUN_8006b240();
				FUN_80061ea0();
				FUN_80011214();
				Front_DirtyRectText(XMid, 240, load);
				RGBRed();
				FUN_8006b26c();
			}
		}

		OldMCState = 24;
		return;
	}

	if (OldMCState != 26)
		Front_RestoreBackground();

	{
		int w = OldMCState;
		int v = w;

		if (v != MCState) {
			if (v == 15)
				goto set;

			if (v == 24) {
				OldMCState = 25;
				return;
			}

			if (v == 25) {
				OldMCState = 26;
				return;
			}

			if (w != 26) {
				OldMCState = 24;
				return;
			}

			FUN_80011570();
		set:
			OldMCState = MCState;
		}
	}

	Mess_SetScale(200);
	Mess_SetTextJustify__FUc(0);

	switch (MCState) {
		case 15:
		case 16:
			FUN_80011214();
			Front_DirtyRectText(XMid, 180, GameText.f200);
			RGBRed();
			Front_DirtyRectText(XMid, 280, GameText.f1f8);
			Front_DirtyRectText(XMid, 332, GameText.f1fc);
			break;
		case 17:
			RGBRed();
			Front_DirtyRectText(XMid, 180, GameText.f1c8);
			Front_DirtyRectText(XMid, 232, GameText.f1cc);
			Front_DirtyRectText(XMid, 284, GameText.f1d0);
			Front_DirtyRectText(XMid, 336, GameText.f1d4);
			break;
		case 18:
			RGBRed();
			Front_DirtyRectText(XMid, 174, GameText.f104);
			Front_DirtyRectText(XMid, 226, GameText.f108);
			Front_DirtyRectText(XMid, 278, GameText.f10c);
			break;
		case 19:
			RGBRed();

			if (GLoadAction == 2)
				Front_DirtyRectText(XMid, 220, GameText.f10);
			else
				Front_DirtyRectText(XMid, 220, GameText.f0c);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.f180);

			break;
		case 20:
			if (!pVertMenu) {
				pVertMenu = new VerticalMenu(100, 398, 15, VertMenuItem,
				                                NumVertMenuItems < 6 ? NumVertMenuItems : 5, 0, 0, 0);
				FUN_8008b900(msg, GameText.f128, GameText.f134);
				Front_DirtyRectText(XMid, 380, msg);
			}

			for (i = 0; i < 5; i++) {
				if (i != pVertMenu->getSelected() || VertMenuTopChanged)
					pVertMenu->setString(i, VertMenuItem[VertMenuTopIndex + i]);
			}

			if (NumVertMenuItems > 5 && ((TTime / 20) & 3)) {
				FUN_80011214();

				{
					int up = VertMenuTopIndex > 0;
					int down = VertMenuTopIndex < NumVertMenuItems - 5;

					if (up && down)
						FUN_8008b900(msg, GameText.f128, GameText.f134);
					else
						FUN_8008b900(msg, GameText.f128,
						             up ? GameText.f12c : GameText.f130);
				}

				Front_DirtyRectText(XMid, 380, msg);
			}

			pVertMenu->draw();

			if (MCResolution == 512)
				Front_PrintGoBackSelect();

			break;
		case 21:
			FUN_80011214();
			Front_DirtyRectText(XMid, 220, s_loading_video_file);
			RGBRed();
			Front_DirtyRectText(XMid, 280, GameText.f1f8);
			Front_DirtyRectText(XMid, 332, GameText.f1fc);
			break;
		case 22:
			RGBRed();
			Front_DirtyRectText(XMid, 220, GameText.f244);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.f180);

			break;
		case 23:
			RGBRed();
			Front_DirtyRectText(XMid, 220, s_Video_loaded);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText_180[0]);

			break;
		case 27:
			break;
	}
}
