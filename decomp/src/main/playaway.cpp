/* PlayAway__Fv (0x80025e00) flags=-fstructure-noalias */
/* C++ (MAIN.CPP). PlayAway (called from LaunchGame). The demo splits this into Game_Init (the
   fuzzy twin of the first 490 words), System_Logic and a 216-byte PlayAway loop. THPS1 writes the
   init out in place: its error exits `goto done` straight to the shutdown, which keeps the first
   EndCode test in the init's last block. System_Logic is an inline. Sets up the level (video restart/replay loading, the
   powerup visibility, settling the cameras), runs the frame loop until EndCode is set, then shuts
   the game down and unloads the fonts. */
typedef unsigned char uchar;
typedef unsigned long ulong;

struct SButton {
	uchar Pressed;
	uchar Triggered;
	int PressedTime;
	int ReleasedTime;
	int TriggeredTime;
};

struct SControl {
	SButton Triangle, Square, Circle, X;
	SButton LeftOne, LeftTwo, RightOne, RightTwo;
	SButton Left, Right, Up, Down;
	SButton AnalogueLeft, AnalogueRight, Start, Select;
	SButton Crouch, Jump, SmartBomb, SelectWeapon;
	int rest[11];
};

struct SHighlight {
	short FromFrame;
	short ToFrame;
	int Score;
};

struct SVideoHead {
	ulong NumFrames;
	SHighlight Highlights[5];
	short NumSkaters;
	short Skater;
	short Skater2;
	short Level;
	short GGame;
	short Checksum;
	char VideoFlags;
	char Padding[7];
};

struct SVideoInfo {
	int VideoMode;
	ulong *pBlockStart;
	ulong *pStreamStart;
	ulong *pStreamEnd;
	ulong *pStream;
	ulong Bits;
	int BitsLeft;
	ulong NumFrames;
	int pad20[11];
	int ReplayHighlight;
	SHighlight Highlights[5];
	int pad78[18];
};

struct SVideoRestart {
	int NumSkaters;
	int Skater;
	int Skater2;
	int Level;
	int GGame;
};

struct SLevel {
	int pad0[3];
	int isCompetition;
	int pad10[20];
	int m60;
	int pad64[5];
};

struct CBody {
	char pad0[0x1c];
	CBody *pNext;
	char pad20[0x330 - 0x20];
	int m330;
};

struct CPowerUp {
	char pad0[0x15c];
	short Visible;
};

struct CVector {
	int vx, vy, vz;
	void Set(int x, int y, int z) {
		vx = x;
		vy = y;
		vz = z;
	}
};

struct CSkater {
	char pad0[0x6c];
	CVector mVel;
	char pad78[0x6c4 - 0x78];
	int m6c4;
	int m6c8;
	int m6cc;
	char pad6d0[0x734 - 0x6d0];
	int Dead;
	char pad738[0x7d8 - 0x738];
	CVector m7d8;
	char pad7e4[0xa64 - 0x7e4];
	int ma64;
};

extern "C" {
	extern volatile unsigned Xblanks; /* Xblanks */
	extern volatile unsigned Vblanks; /* Vblanks */
	extern int DemoTimer;               /* DemoTimer */
	extern int VblankUpdateBar;               /* VblankUpdateBar */
	extern int GVideoReplay;               /* GVideoReplay */
	extern CBody *MechList;            /* MechList */
	extern int GLevel;               /* GLevel */
	extern CBody *CameraList;            /* CameraList */
	extern unsigned GameFade;          /* GameFade */
	extern int GamePaused;
	extern int M3d_WireframeMode;
	extern short M3d_OTPushback;         /* M3d_OTPushback */
	extern SVideoRestart GVideoRestart; /* GVideoRestart */
	extern SVideoInfo GVInfo1;    /* GVInfo1 */
	extern SVideoInfo GVInfo2;    /* GVInfo2 */
	extern uchar DemoName[];       /* DemoName */
	extern SLevel Levels[];      /* Levels */
	extern SControl Pad[2];   /* Pad */
	extern char s_ingame1_fnt_8009f584[];        /* "ingame1.fnt" */
	extern char s_memory_card_error[];        /* "memory card error" */
	extern char s_press_x_to_continue[];        /* "press x to continue" */

	int EndCode;       /* EndCode */
	int GGame;       /* GGame */
	int GNumberOfPlayers;       /* GNumberOfPlayers */
	char GVideoFlags;      /* GVideoFlags */
	CPowerUp *GPowS; /* GPowS */
	CPowerUp *GPowK; /* GPowK */
	CPowerUp *GPowA; /* GPowA */
	CPowerUp *GPowT; /* GPowT */
	CPowerUp *GPowE; /* GPowE */
	CPowerUp *GPowTape; /* GPowTape */
	int GState;       /* GState */
	int GSpin;
	int GSpinSpeed;
	CSkater *GSkater; /* GSkater[0] */
	CSkater *DAT_800d17cc; /* GSkater[1] */
	int GHorsePlayer;      /* GHorsePlayer */
	int GClear1;      /* GClear1 */
	int GClear2;      /* GClear2 */
	int GHorseClearInput2;
	int Cheat_ScreenShots;
	int GameFrozen;
	int Cheat_FPS;
	int GHorseWordChosen;
	int GHorseDoneStart;
	int GStartTime;
	int GSkaterRegion;
	int DAT_800d1798;

	void FUN_8002566c(void); /* PlayAway_Init */
	void Pause__Fi(int);
	void FUN_80025400(void); /* FPS_Init */
	void FUN_800109b0(void); /* Crowd_Init */
	void FUN_800109b8(void); /* Crowd_Start */
	void ResetMainDemoTimer__Fv(void); /* ResetMainDemoTimer */
	void FUN_80011628(int);  /* Front_SetLoadAction */
	int Card_Load__F9EFileTypePUlT1(int, ulong *, ulong *);
	void Mess_SetCurrentFont__FPc(char *);
	void FUN_8006b240(void);                              /* HiRes_ClearOTag */
	void FUN_80061ea0(void);                              /* ResetOT */
	void SetClippingBox__Fiiii(int, int, int, int);                /* SetClippingBox */
	void DrawColoredBox__FiiiiUcUcUc(int, int, int, int, int, int, int); /* DrawColoredBox */
	void Mess_SetTextJustify__FUc(int);                               /* Mess_SetTextJustify */
	void Mess_SetScale__Fi(int);
	void Mess_SetRGB__FUcUcUci(int, int, int, int);
	void Mess_DrawText__FiiPCciUiib(int, int, char *, int, int, int, int); /* Mess_DrawText */
	void FUN_8006b26c(void);                                 /* HiRes_DrawOTag */
	void Pad_ClearTriggers__FP8SControl(SControl *);
	int FUN_80070ff4(void); /* Pad_Update */
	int Card_LoadReplayFromCD__FPUlT0Pc(ulong *, ulong *, uchar *);
	int Career_Got__Fi(int);                     /* Career_Got */
	void Video_Start__FP6CBruce10EVideoModeii(CBody *, int, int, int); /* Video_Start */
	void Video_SyncObsIfNeeded__Fv(void);
	void Flash_Reset__Fv(void);   /* Flash_Reset */
	void Video_Cleanup__Fv(void);   /* Video_Cleanup */
	int FUN_8007cd64(CBody *); /* InCompetitionIntro */
	void FUN_800920b4(int);
	void Ob_AI__FPP5CBody(CBody **); /* Ob_AI */
	void FUN_800242cc(void);
	void SFX_Stop__FUi(int);
	void FUN_8001a1c8(void);
	void Front_SwitchOnEndLevelMenu__Fv(void);
	void FUN_80010bb8(void);
	void FUN_80024258(void);
	void Pad_Remap__Fv(void); /* Pad_Restore */
	void Pad_Restore__Fv(void); /* Pad_Remap */
	void Pad_ClearAllOne__Fi(int);
	void Video_HandleStepping__Fv(void);
	void Game_Logic__Fv(void); /* Game_Logic */
	void FUN_80048fec(void); /* Db_WaitRender */
	void FUN_80025440(void);
	void FUN_80024fa4(void); /* Game_Display */
	void Panel_DrawFlatShadedPoly__FiiiiUcUcUci(int, int, int, int, int, int, int, int);
	void Front_PrintDemo__Fv(void);
	void FUN_8002357c(int);
	void Db_Render__Fv(void);
	void FUN_8001a5e0(void);
	void Front_SwitchOnStartLevelMenu__Fv(void);
	int IsDead__C5CBody(CBody *);
	void Spool_Sync__Fv(void);
	void Front_SaveGameState__Fv(void);
	void FUN_8008c468(int);
	void FUN_800491e8(void); /* Db_Init */
	void FUN_80010ac0(void); /* Crowd_Stop */
	void FUN_8001f138(int);
	void Mess_UnloadFont__Fv(void);
}

static inline void System_Logic(void) {
	SControl TempPad;

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	Pad_Remap__Fv();
	FUN_80070ff4();
	Pad_Restore__Fv();

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	if (GClear1)
		Pad_ClearAllOne__Fi(0);

	if (GClear2)
		Pad_ClearAllOne__Fi(1);
}

void PlayAway(void) {
	int XStart;

	FUN_8002566c();

	XStart = Xblanks;
	Pause__Fi(1);
	FUN_80025400();
	FUN_800109b0();
	FUN_800109b8();

	EndCode = 0;
	ResetMainDemoTimer__Fv();

	if (GVideoRestart.NumSkaters > 0 && DemoTimer) {
		int LoadError;

		FUN_80011628(0);

		VblankUpdateBar = 1;

		if (GVideoRestart.NumSkaters == 1)
			LoadError = Card_Load__F9EFileTypePUlT1(1, GVInfo1.pBlockStart, 0);
		else
			LoadError =
			    Card_Load__F9EFileTypePUlT1(1, GVInfo1.pBlockStart, GVInfo2.pBlockStart);

		VblankUpdateBar = 0;

		if (LoadError) {
			Mess_SetCurrentFont__FPc(s_ingame1_fnt_8009f584);
			FUN_8006b240();
			FUN_80061ea0();
			SetClippingBox__Fiiii(0, 0, 512, 240);
			DrawColoredBox__FiiiiUcUcUc(0, 0, 512, 240, 0, 0, 0);
			Mess_SetTextJustify__FUc(0);
			Mess_SetScale__Fi(255);
			Mess_SetRGB__FUcUcUci(150, 100, 25, 0);
			Mess_DrawText__FiiPCciUiib(256, 110, s_memory_card_error, 1, 0, 256, 0);
			Mess_DrawText__FiiPCciUiib(256, 130, s_press_x_to_continue, 1, 0, 256, 0);
			FUN_8006b26c();
			Pad_ClearTriggers__FP8SControl(&Pad[0]);
			Pad_ClearTriggers__FP8SControl(&Pad[1]);

			while (!Pad[0].X.Triggered && !Pad[1].X.Triggered &&
			       !Pad[0].Start.Triggered && !Pad[1].Start.Triggered)
				FUN_80070ff4();

			EndCode = 9;
			goto done;
		}

		{
			SVideoHead *VH = (SVideoHead *)(GVInfo1.pBlockStart + 128);

			GVInfo1.NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1.Highlights[i] = VH->Highlights[i];
		}

		if (GVideoRestart.NumSkaters == 2) {
			SVideoHead *VH = (SVideoHead *)(GVInfo2.pBlockStart + 128);

			GVInfo2.NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo2.Highlights[i] = VH->Highlights[i];
		}

		GVideoRestart.NumSkaters = -GVideoRestart.NumSkaters;
		GVideoReplay = -1;
	}

	if (!DemoTimer) {
		int LoadError;

		VblankUpdateBar = 1;
		LoadError = Card_LoadReplayFromCD__FPUlT0Pc(GVInfo1.pBlockStart,
		                                            GVInfo2.pBlockStart, DemoName);
		DemoName[4]++;

		if (DemoName[4] > 'J')
			DemoName[4] = 'A';

		VblankUpdateBar = 0;

		if (LoadError) {
			EndCode = 9;
			goto done;
		}

		{
			SVideoHead *VH = (SVideoHead *)(GVInfo1.pBlockStart + 128);

			GVideoRestart.NumSkaters = VH->NumSkaters;
			GVideoRestart.Skater = VH->Skater;
			GVideoRestart.Skater2 = VH->Skater2;
			GVideoRestart.Level = VH->Level;
			GVideoRestart.GGame = VH->GGame;
			GVInfo1.NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo1.Highlights[i] = VH->Highlights[i];
		}

		if (GVideoRestart.NumSkaters == 2) {
			SVideoHead *VH = (SVideoHead *)(GVInfo2.pBlockStart + 128);

			GVInfo2.NumFrames = VH->NumFrames;

			for (int i = 0; i < 5; i++)
				GVInfo2.Highlights[i] = VH->Highlights[i];
		}

		GVideoRestart.NumSkaters = -GVideoRestart.NumSkaters;
		GVideoReplay = -1;
	}

	if (!GVideoReplay) {
		if (GGame == 1) {
			GVideoFlags = 0;

			if (Career_Got__Fi(3))
				GVideoFlags |= 1;

			if (Career_Got__Fi(2))
				GVideoFlags |= 2;
		} else {
			GVideoFlags = 3;
		}
	} else {
		GVideoFlags = ((SVideoHead *)(GVInfo1.pBlockStart + 128))->VideoFlags;
	}

	if (GGame == 1 && !Levels[GLevel].isCompetition) {
		if (GVideoFlags & 2) {
			GPowS->Visible = 1;
			GPowK->Visible = 1;
			GPowA->Visible = 1;
			GPowT->Visible = 1;
			GPowE->Visible = 1;
		}

		if (GVideoFlags & 1)
			GPowTape->Visible = 1;
	}

	if (GVideoReplay) {
		CBody *pSkater = MechList;
		CBody *pSkater2 = pSkater->pNext;

		if (GVideoReplay > 0) {
			GVInfo1.ReplayHighlight = GVideoReplay;
			GVInfo2.ReplayHighlight = GVideoReplay;
			Video_Start__FP6CBruce10EVideoModeii(pSkater, 2, GVInfo1.Highlights[GVideoReplay - 1].FromFrame,
			             GVInfo1.Highlights[GVideoReplay - 1].ToFrame);

			if (pSkater2)
				Video_Start__FP6CBruce10EVideoModeii(pSkater2, 2, GVInfo1.Highlights[GVideoReplay - 1].FromFrame,
				             GVInfo1.Highlights[GVideoReplay - 1].ToFrame);

			Video_SyncObsIfNeeded__Fv();
		} else {
			GVideoReplay = 0;
			GVInfo1.ReplayHighlight = 0;
			GVInfo2.ReplayHighlight = 0;
			Video_Start__FP6CBruce10EVideoModeii(pSkater, 2, 1, 0x7fffffff);

			if (pSkater2)
				Video_Start__FP6CBruce10EVideoModeii(pSkater2, 2, 1, 0x7fffffff);
		}

		GState = 0;
		Flash_Reset__Fv();

		MechList->m330 = Xblanks + 216030;

		if (MechList->pNext)
			MechList->pNext->m330 = Xblanks + 216030;
	} else {
		Video_Cleanup__Fv();

		{
			CBody *pSkater = MechList;
			CBody *pSkater2 = pSkater->pNext;

			if (!GVInfo1.VideoMode && !FUN_8007cd64(CameraList)) {
				Video_Start__FP6CBruce10EVideoModeii(pSkater, 1, 1, 0x7fffffff);

				if (pSkater2)
					Video_Start__FP6CBruce10EVideoModeii(pSkater2, 1, 1, 0x7fffffff);

				((SVideoHead *)(GVInfo1.pBlockStart + 128))->VideoFlags = GVideoFlags;
			}
		}
	}

	XStart = Xblanks;

	if (GNumberOfPlayers == 1)
		FUN_800920b4(1);

	{
		CSkater *p = GSkater;
		int x = p->m7d8.vx, y = p->m7d8.vy, z = p->m7d8.vz;

		p->mVel.vx = x * 20;
		p->mVel.vy = y * 20;
		p->mVel.vz = z * 20;
	}

	for (int i = 0; i < 10; i++)
		Ob_AI__FPP5CBody(&CameraList);

	GameFade = 0;
	GSkater->mVel.vx = 0;
	GSkater->mVel.vy = 0;
	GSkater->mVel.vz = 0;

	while (!EndCode) {
		unsigned StartVblanks = Vblanks;

		FUN_800242cc();

		if (!GSkater->Dead && (!DAT_800d17cc || !DAT_800d17cc->Dead) && GState == 1) {
			if (--GSpin <= 0) {
				if (GGame != 7 && GSkater->ma64) {
					GSpinSpeed = 0;
					GSpin = 1;
				} else {
					if (GSkater->m6c8) {
						SFX_Stop__FUi(GSkater->m6c4);

						GSkater->m6c8 = 0;
					}

					if (GSkater->m6cc) {
						SFX_Stop__FUi(GSkater->m6cc);

						GSkater->m6cc = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->m6c8) {
						SFX_Stop__FUi(DAT_800d17cc->m6c8);

						DAT_800d17cc->m6c8 = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->m6cc) {
						SFX_Stop__FUi(DAT_800d17cc->m6cc);

						DAT_800d17cc->m6cc = 0;
					}

					GState = 0;

					if (Levels[GLevel].isCompetition && GGame == 1)
						FUN_8001a1c8();
					else
						Front_SwitchOnEndLevelMenu__Fv();
				}
			}
		}

		FUN_80010bb8();
		FUN_80024258();
		System_Logic();
		Video_HandleStepping__Fv();

		if (GState == 1 && !GamePaused) {
			if (!GSkater->Dead) {
				uchar c = Pad[0].Circle.Pressed;

				Pad_ClearAllOne__Fi(0);

				Pad[0].Down.Pressed = 1;
				Pad[0].Circle.Pressed = c;
			}

			if (GNumberOfPlayers == 2 && !DAT_800d17cc->Dead) {
				uchar c = Pad[1].Circle.Pressed;

				Pad_ClearAllOne__Fi(1);

				Pad[1].Down.Pressed = 1;
				Pad[1].Circle.Pressed = c;
			}
		}

		if ((GNumberOfPlayers == 1 || (GGame == 7 && GHorseClearInput2)) && !Cheat_ScreenShots)
			Pad_ClearAllOne__Fi(1);

		Game_Logic__Fv();

		if (EndCode)
			break;

		Game_Logic__Fv();

		if (EndCode)
			break;

		FUN_80048fec();
		FUN_80025440();
		FUN_80024fa4();

		if (!GameFrozen && Cheat_FPS)
			Panel_DrawFlatShadedPoly__FiiiiUcUcUci(448, 208, 54, 18, 0, 0, 0, 0);

		if (!DemoTimer)
			Front_PrintDemo__Fv();

		FUN_8002357c(Levels[GLevel].m60 && (GNumberOfPlayers == 1 || GGame == 7)
		                 ? M3d_WireframeMode
						 : 0);

		M3d_WireframeMode = 0;

		if (Vblanks == StartVblanks)
			Pause__Fi(1);

		Db_Render__Fv();

		if (Vblanks == StartVblanks + 1)
			Pause__Fi(1);

		if (GGame == 7) {
			if (!GHorseWordChosen) {
				GHorseDoneStart = 1;
				GHorseWordChosen = 1;
				FUN_8001a5e0();
			} else if (!GHorseDoneStart) {
				GHorseDoneStart = 1;
				Front_SwitchOnStartLevelMenu__Fv();
			}
		}

		if (IsDead__C5CBody(MechList))
			EndCode = 2;
	}
done:
	FUN_800920b4(3);
	Spool_Sync__Fv();

	M3d_OTPushback = 1;
	Front_SaveGameState__Fv();
	FUN_80048fec();
	FUN_8008c468(0);
	FUN_800491e8();
	FUN_80010ac0();
	FUN_8001f138(DAT_800d1798);
	Mess_UnloadFont__Fv();
	FUN_8001f138(GSkaterRegion);
	Mess_UnloadFont__Fv();
	FUN_8001f138(GStartTime);
	Mess_UnloadFont__Fv();
}
