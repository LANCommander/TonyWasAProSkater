/* PlayAway__Fv (0x80025e00) flags=-fstructure-noalias */
/* C++ (MAIN.CPP). PlayAway (called from LaunchGame). The demo splits this into Game_Init (the
   fuzzy twin of the first 490 words), System_Logic and a 216-byte PlayAway loop. THPS1 writes the
   init out in place: its error exits `goto done` straight to the shutdown, which keeps the first
   EndCode test in the init's last block. System_Logic is an inline. Sets up the level (video restart/replay loading, the
   powerup visibility, settling the cameras), runs the frame loop until EndCode is set, then shuts
   the game down and unloads the fonts. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/video.h"
#include "game/career.h"
#include "game/bruce.h"
#include "game/powerup.h"
#include "game/pad.h"
#include "game/mess.h"
#include "game/card.h"
#include "game/flash.h"
#include "game/front.h"
#include "game/main.h"
#include "game/ob.h"
#include "game/panel.h"
#include "game/sfx.h"
#include "game/shellutils.h"
#include "game/spool.h"
#include "game/utils.h"
#include "game/psyq.h"
#include "game/camera.h"
#include "game/crowd.h"
#include "game/db.h"
#include "game/gamefmv.h"
#include "game/hires.h"
#include "game/rfront.h"

extern "C" {
	extern volatile unsigned Xblanks; /* Xblanks */
	extern volatile unsigned Vblanks; /* Vblanks */
	extern int DemoTimer;             /* DemoTimer */
	extern int VblankUpdateBar;       /* VblankUpdateBar */
	extern int GVideoReplay;          /* GVideoReplay */
	extern CBody *MechList;           /* MechList */
	extern int GLevel;                /* GLevel */
	extern CBody *CameraList;         /* CameraList */
	extern unsigned GameFade;         /* GameFade */
	extern int GamePaused;
	extern int M3d_WireframeMode;
	extern short M3d_OTPushback;        /* M3d_OTPushback */
	extern SVideoRestart GVideoRestart; /* GVideoRestart */
	extern SVideoInfo GVInfo1;          /* GVInfo1 */
	extern SVideoInfo GVInfo2;          /* GVInfo2 */
	extern uchar DemoName[];            /* DemoName */
	extern SLevel Levels[];             /* Levels */
	extern SControl Pad[2];             /* Pad */

	int EndCode;          /* EndCode */
	int GGame;            /* GGame */
	int GNumberOfPlayers; /* GNumberOfPlayers */
	char GVideoFlags;     /* GVideoFlags */
	CPowerUp *GPowS;      /* GPowS */
	CPowerUp *GPowK;      /* GPowK */
	CPowerUp *GPowA;      /* GPowA */
	CPowerUp *GPowT;      /* GPowT */
	CPowerUp *GPowE;      /* GPowE */
	CPowerUp *GPowTape;   /* GPowTape */
	int GState;           /* GState */
	int GSpin;
	int GSpinSpeed;
	CBruce *GSkater;      /* GSkater[0] */
	CBruce *DAT_800d17cc; /* GSkater[1] */
	int GHorsePlayer;     /* GHorsePlayer */
	int GClear1;          /* GClear1 */
	int GClear2;          /* GClear2 */
	int GHorseClearInput2;
	int Cheat_ScreenShots;
	int GameFrozen;
	int Cheat_FPS;
	int GHorseWordChosen;
	int GHorseDoneStart;
	int GStartTime;
	int GSkaterRegion;
	int DAT_800d1798;
}

static inline void System_Logic(void) {
	SControl TempPad;

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	Pad_Remap();
	Pad_Update();
	Pad_Restore();

	if (GGame == 7 && GHorsePlayer == 1) {
		TempPad = Pad[0];
		Pad[0] = Pad[1];
		Pad[1] = TempPad;
	}

	if (GClear1)
		Pad_ClearAllOne(0);

	if (GClear2)
		Pad_ClearAllOne(1);
}

void PlayAway(void) {
	int XStart;

	PlayAway_Init();

	XStart = Xblanks;
	Pause(1);
	FPS_Init();
	Crowd_Init();
	Crowd_Start();

	EndCode = 0;
	ResetMainDemoTimer();

	if (GVideoRestart.NumSkaters > 0 && DemoTimer) {
		int LoadError;

		Front_SetLoadAction(LOAD_REPLAY);

		VblankUpdateBar = 1;

		if (GVideoRestart.NumSkaters == 1)
			LoadError = Card_Load(1, GVInfo1.pBlockStart, 0);
		else
			LoadError = Card_Load(1, GVInfo1.pBlockStart, GVInfo2.pBlockStart);

		VblankUpdateBar = 0;

		if (LoadError) {
			Mess_SetCurrentFont("ingame1.fnt");
			HiRes_ClearOTag();
			ResetOT();
			SetClippingBox(0, 0, 512, 240);
			DrawColoredBox(0, 0, 512, 240, 0, 0, 0);
			Mess_SetTextJustify(0);
			Mess_SetScale(255);
			Mess_SetRGB(150, 100, 25, 0);
#ifdef PC_PORT
			Mess_DrawText(256, 110, (char *)"load failed", 1, 0, 256, 0);
#else
			Mess_DrawText(256, 110, "memory card error", 1, 0, 256, 0);
#endif
			Mess_DrawText(256, 130, "press x to continue", 1, 0, 256, 0);
			HiRes_DrawOTag();
			Pad_ClearTriggers(&Pad[0]);
			Pad_ClearTriggers(&Pad[1]);

			while (!Pad[0].MenuAccept.Triggered && !Pad[1].MenuAccept.Triggered &&
			       !Pad[0].MenuStart.Triggered && !Pad[1].MenuStart.Triggered)
				Pad_Update();

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
		LoadError = Card_LoadReplayFromCD(GVInfo1.pBlockStart, GVInfo2.pBlockStart, DemoName);
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

			if (Career_Got(3))
				GVideoFlags |= 1;

			if (Career_Got(2))
				GVideoFlags |= 2;
		} else {
			GVideoFlags = 3;
		}
	} else {
		GVideoFlags = ((SVideoHead *)(GVInfo1.pBlockStart + 128))->VideoFlags;
	}

	if (GGame == 1 && !Levels[GLevel].isCompetition) {
		if (GVideoFlags & 2) {
			GPowS->mLifetime = 1;
			GPowK->mLifetime = 1;
			GPowA->mLifetime = 1;
			GPowT->mLifetime = 1;
			GPowE->mLifetime = 1;
		}

		if (GVideoFlags & 1)
			GPowTape->mLifetime = 1;
	}

	if (GVideoReplay) {
		CBody *pSkater = MechList;
		CBody *pSkater2 = (CBody *)pSkater->mNextItem;

		if (GVideoReplay > 0) {
			GVInfo1.ReplayHighlight = GVideoReplay;
			GVInfo2.ReplayHighlight = GVideoReplay;
			Video_Start((CBruce *)pSkater, (EVideoMode)2,
			            GVInfo1.Highlights[GVideoReplay - 1].FromFrame,
			            GVInfo1.Highlights[GVideoReplay - 1].ToFrame);

			if (pSkater2)
				Video_Start((CBruce *)pSkater2, (EVideoMode)2,
				            GVInfo1.Highlights[GVideoReplay - 1].FromFrame,
				            GVInfo1.Highlights[GVideoReplay - 1].ToFrame);

			Video_SyncObsIfNeeded();
		} else {
			GVideoReplay = 0;
			GVInfo1.ReplayHighlight = 0;
			GVInfo2.ReplayHighlight = 0;
			Video_Start((CBruce *)pSkater, (EVideoMode)2, 1, 0x7fffffff);

			if (pSkater2)
				Video_Start((CBruce *)pSkater2, (EVideoMode)2, 1, 0x7fffffff);
		}

		GState = 0;
		Flash_Reset();

		((CBruce *)MechList)->mTime = Xblanks + 216030;

		if (MechList->mNextItem)
			((CBruce *)MechList->mNextItem)->mTime = Xblanks + 216030;
	} else {
		Video_Cleanup();

		{
			CBody *pSkater = MechList;
			CBody *pSkater2 = (CBody *)pSkater->mNextItem;

			if (!GVInfo1.VideoMode && !((CCamera *)CameraList)->InCompetitionIntro()) {
				Video_Start((CBruce *)pSkater, (EVideoMode)1, 1, 0x7fffffff);

				if (pSkater2)
					Video_Start((CBruce *)pSkater2, (EVideoMode)1, 1, 0x7fffffff);

				((SVideoHead *)(GVInfo1.pBlockStart + 128))->VideoFlags = GVideoFlags;
			}
		}
	}

	XStart = Xblanks;

	if (GNumberOfPlayers == 1)
		PadEnableCom(1);

	{
		CBruce *p = GSkater;
		int x = p->mFront.vx, y = p->mFront.vy, z = p->mFront.vz;

		p->mVel.vx = x * 20;
		p->mVel.vy = y * 20;
		p->mVel.vz = z * 20;
	}

	for (int i = 0; i < 10; i++)
		Ob_AI(&CameraList);

	GameFade = 0;
	GSkater->mVel.vx = 0;
	GSkater->mVel.vy = 0;
	GSkater->mVel.vz = 0;

	while (!EndCode) {
		unsigned StartVblanks = Vblanks;

		PollHost();

		if (!GSkater->mPhysicsState && (!DAT_800d17cc || !DAT_800d17cc->mPhysicsState) &&
		    GState == 1) {
			if (--GSpin <= 0) {
				if (GGame != 7 && GSkater->mPanel.mLandedTotal) {
					GSpinSpeed = 0;
					GSpin = 1;
				} else {
					if (GSkater->mGrindSnd) {
						SFX_Stop(GSkater->mLoopSnd);

						GSkater->mGrindSnd = 0;
					}

					if (GSkater->mRollSnd) {
						SFX_Stop(GSkater->mRollSnd);

						GSkater->mRollSnd = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->mGrindSnd) {
						SFX_Stop(DAT_800d17cc->mGrindSnd);

						DAT_800d17cc->mGrindSnd = 0;
					}

					if (DAT_800d17cc && DAT_800d17cc->mRollSnd) {
						SFX_Stop(DAT_800d17cc->mRollSnd);

						DAT_800d17cc->mRollSnd = 0;
					}

					GState = 0;

					if (Levels[GLevel].isCompetition && GGame == 1)
						Front_SwitchOnEndHeat();
					else
						Front_SwitchOnEndLevelMenu();
				}
			}
		}

		Crowd_Update();
		CalcPolyBufferEnd();
		System_Logic();
		Video_HandleStepping();

		if (GState == 1 && !GamePaused) {
			if (!GSkater->mPhysicsState) {
				uchar c = Pad[0].Grab.Pressed;

				Pad_ClearAllOne(0);

				Pad[0].Down.Pressed = 1;
				Pad[0].Grab.Pressed = c;
			}

			if (GNumberOfPlayers == 2 && !DAT_800d17cc->mPhysicsState) {
				uchar c = Pad[1].Grab.Pressed;

				Pad_ClearAllOne(1);

				Pad[1].Down.Pressed = 1;
				Pad[1].Grab.Pressed = c;
			}
		}

		if ((GNumberOfPlayers == 1 || (GGame == 7 && GHorseClearInput2)) && !Cheat_ScreenShots)
			Pad_ClearAllOne(1);

		Game_Logic();

		if (EndCode)
			break;

		Game_Logic();

		if (EndCode)
			break;

		Db_WaitRender();
		FPS_Display();
		::Display();

		if (!GameFrozen && Cheat_FPS)
			Panel_DrawFlatShadedPoly(448, 208, 54, 18, 0, 0, 0, 0);

		if (!DemoTimer)
			Front_PrintDemo();

		GameFMV_SyncFMV(
		    Levels[GLevel].FMV && (GNumberOfPlayers == 1 || GGame == 7) ? M3d_WireframeMode : 0);

		M3d_WireframeMode = 0;

		if (Vblanks == StartVblanks)
			Pause(1);

		Db_Render();

		if (Vblanks == StartVblanks + 1)
			Pause(1);

		if (GGame == 7) {
			if (!GHorseWordChosen) {
				GHorseDoneStart = 1;
				GHorseWordChosen = 1;
				Front_PrintHorseLetters();
			} else if (!GHorseDoneStart) {
				GHorseDoneStart = 1;
				Front_SwitchOnStartLevelMenu();
			}
		}

		if (MechList->IsDead())
			EndCode = 2;
	}
done:
	PadEnableCom(3);
	Spool_Sync();

	M3d_OTPushback = 1;
	Front_SaveGameState();
	Db_WaitRender();
	DrawSync(0);
	Db_Init();
	Crowd_Stop();
	Mess_SetCurrentFont(DAT_800d1798);
	Mess_UnloadFont();
	Mess_SetCurrentFont(GSkaterRegion);
	Mess_UnloadFont();
	Mess_SetCurrentFont(GStartTime);
	Mess_UnloadFont();
}
