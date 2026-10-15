/* Front_LoadGame__FP9SSaveGame (0x8001ccf8) */
#include "game/card.h"
#include "game/career.h"
#include "game/m3d.h"
#include "game/shellutils.h"

extern char *TrigFileName;
extern SStats Stats[];
extern SLevel Levels[];
extern SPSXRegion PSXRegion[];
extern int SelectedSkater;
extern int GGame;
extern int SelectedSkater2;
extern int Cheat_BigHead;
extern int GNumberOfPlayers;
extern Sprite *pScoreSprite;
extern Sprite *pSpecialSprite;
extern Sprite *pBarSprite;
extern Sprite *DAT_800d1858;
extern char CurrentItemsFileRegion;
extern char CurrentMedalsFileRegion;
extern int BackOut;
extern int GLevel;
extern int GHeadPart;
extern int GHeadUp;

void Bit_Init();
void CreateSkatersAndCameras();
void Trig_ExecuteRestart();
void Trig_InitialParseTRGFile(char *);
void SFX_Init(char *);
extern "C" void FUN_8004b0e4(char *);
extern "C" void FUN_8004b180(void);
extern "C" void FUN_8004bb5c(void);
void FileIO_SetSectorsPerFrame(int);
extern "C" void FUN_8005dc8c(void);
extern "C" void FUN_8006c4c4(void);
int Spool_PSX(char *, int, int);
void Font_Init();
SLevel *Front_FindLevel(char *);
void Front_SaveGameState();
void Redbook_InitialRand(int);
void SFX_SetReverbType(uchar);
void Spool_ClearAllPSXs();
void Spool_Sync();
void Trig_SetRestart(char *);

void Front_LoadGame(SSaveGame *p) {
	int s2, t1, t2, u1, u2, i1;
	SStats *q;

	TrigFileName = p->State.TrigFileName;
	Redbook_InitialRand(0x12a6cc58);
	FileIO_SetSectorsPerFrame(2000);
	SFX_Init("skate2.vab");
	FUN_8004b0e4(Levels[GLevel].pVab);
	SFX_SetReverbType(0);

	pScoreSprite = new Sprite("scorep.bmp", 1, 256, 256);
	pSpecialSprite = new Sprite("special.bmp", 1, 256, 256);
	pBarSprite = new Sprite("spec_bar.bmp", 1, 256, 256);
	DAT_800d1858 = new Sprite("paused.bmp", 1, 64, 256);
	i1 = Spool_PSX("bits", 0, 0);
	PSXRegion[i1].Protected = 1;
	s2 = Spool_PSX("items", 0, 0);
	PSXRegion[s2].Protected = 1;
	CurrentItemsFileRegion = s2;
	Spool_Sync();
	FileIO_SetSectorsPerFrame(4);
	FUN_8006c4c4();
	Font_Init();
	Bit_Init();
	Spool_ClearAllPSXs();

	if (Levels[GLevel].isCompetition && GGame == 1)
		s2 = Spool_PSX("SkMedals", 0, 0);

	CurrentMedalsFileRegion = s2;

	if (Cheat_BigHead) {
		q = &Stats[SelectedSkater];
		t1 = q->Head;
		t2 = q->Neck;
		GHeadPart = t1;
		GHeadUp = t2;
	}

	if (GNumberOfPlayers == 2 && GGame != 7)
		Spool_PSX(Stats[SelectedSkater].PSXName2, 0, 0);
	else
		Spool_PSX(Stats[SelectedSkater].PSXName, 0, 0);

	Spool_Sync();

	GHeadPart = -1;

	if (GNumberOfPlayers == 2) {
		if (Cheat_BigHead) {
			q = &Stats[SelectedSkater2];
			u1 = q->Neck;
			u2 = q->Head;
			GHeadUp = u1;
			GHeadPart = u2;
		}

		if (GGame != 7)
			Spool_PSX(Stats[SelectedSkater2].PSXName2, 0, 0);
		else
			Spool_PSX(Stats[SelectedSkater2].PSXName, 0, 0);

		Spool_Sync();

		GHeadPart = -1;
	}

	if (Front_FindLevel(TrigFileName)) {
		FUN_8004bb5c();
		FUN_8004b180();
	}

	FileIO_SetSectorsPerFrame(2000);
	Trig_InitialParseTRGFile(TrigFileName);
	FUN_8005dc8c();
	FileIO_SetSectorsPerFrame(4);

	if (BackOut == 0) {
		CreateSkatersAndCameras();

		if (p->State.RestartName[0])
			Trig_SetRestart(p->State.RestartName);

		Trig_ExecuteRestart();
		Front_SaveGameState();
	}
}
