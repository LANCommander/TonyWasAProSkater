/* Game_Logic__Fv (0x80024868) flags=-fstructure-noalias */
/* C++ (MAIN.CPP). Fuzzy demo twin; locals (Frozen) from MAIN.SYM. Runs the per-frame game logic
   on every other frame: objects, tricks, panels, front end and messages, swapping the two pads
   around the front-end update in two-player mode. */
struct SControl {
	int b[91];
};

struct CBody;

struct CBodyPtrs {
	CBody *p;
};

struct SCareer {
	char pad[0x5f0];
	int Money;
};

struct CSkater {
	char pad1d8[0x1d8];
	SCareer *pCareer;
	char pad[0x734 - 0x1dc];
	int Dead;
};

extern "C" {
	extern SControl Pad[2];
	extern int GamePaused[1];

	int EveryOther;
	int DAT_800d183c;
	int TTime;
	int GameFrozen;
	int GGame;
	int GState;
	int GSpin;
	int GSpinSpeed;
	int GHorsePlayer;
	CSkater *GSkater;
	CSkater *DAT_800d17cc;

	extern CBody *BaddyList;
	extern CBody *EnvironmentalObjectList;
	extern CBody *MechList;
	extern CBody *PowerUpList;
	extern CBody *BackgroundList;
	extern CBody *CameraList;

	void Front_Update__Fv(void);
	void Bit_RemoveDeadBits__Fv(void);
	void FUN_8002aa6c(void);
	void Ob_AI__FPP5CBody(CBody **);
	void Ob_QuickAI__FPP5CBody(CBody **);
	void FUN_80048044(void);
	void FileIO_Continue__Fv(void);
	void FUN_8004f8a0(void);
	void FUN_8006c4e8(void);
	void Spool_Continue__Fv(void);
	void Pad_Remap__Fv(void);
	void Pad_Restore__Fv(void);
	void TrickTips_Update__Fv(void);
	void Flash_Update__Fv(void);
	void Font_Reset__Fv(void);
	void Mess_Update__Fv(void);
	void Ob_ClearInputFlags__FP5CBody(CBody *);
	void Ob_MaybeUnSuspendOrCull__Fv(void);
	void Shatter_MaybeMakeGlassShatterSound__Fv(void);
	void TrickOb_Update__Fv(void);
}

void Game_Logic(void) {
	int Frozen;

	DAT_800d183c++;

	if (EveryOther++ & 1)
		return;

	TTime++;
	Font_Reset__Fv();

	Frozen = 0;

	if (GamePaused[0] || GameFrozen)
		Frozen = 1;

	if (!Frozen) {
		Flash_Update__Fv();
		FUN_80048044();
		Ob_ClearInputFlags__FP5CBody(BaddyList);
		Ob_ClearInputFlags__FP5CBody(EnvironmentalObjectList);
		Ob_AI__FPP5CBody(&MechList);

		if (!GamePaused[0]) {
			FUN_8002aa6c();
			FUN_8004f8a0();
			Ob_AI__FPP5CBody(&PowerUpList);
			TrickOb_Update__Fv();
			Ob_QuickAI__FPP5CBody(&EnvironmentalObjectList);
			Ob_QuickAI__FPP5CBody(&BackgroundList);
			Ob_MaybeUnSuspendOrCull__Fv();
			Ob_AI__FPP5CBody(&BaddyList);
			Bit_RemoveDeadBits__Fv();

			if (GGame != 7)
				if (GState == 1 && GSpin < 40 && !GSkater->Dead &&
				    (!DAT_800d17cc || !DAT_800d17cc->Dead)) {
					GSkater->pCareer->Money += GSpinSpeed;

					if (DAT_800d17cc)
						DAT_800d17cc->pCareer->Money += GSpinSpeed;
				}
		}
	} else {
		if (GState == 1)
			Ob_AI__FPP5CBody(&CameraList);
	}

	Shatter_MaybeMakeGlassShatterSound__Fv();
	FUN_8006c4e8();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	Pad_Remap__Fv();
	Front_Update__Fv();
	Pad_Restore__Fv();

	if (GGame == 7 && GHorsePlayer == 1) {
		SControl t = Pad[0];

		Pad[0] = Pad[1];
		Pad[1] = t;
	}

	TrickTips_Update__Fv();
	Mess_Update__Fv();
	Spool_Continue__Fv();
	FileIO_Continue__Fv();
}
