/* FUN_8001ccf8 (0x8001ccf8) */
#include "names.h"

extern char *TrigFileName;
extern char s_skate2_vab[];
extern char s_scorep_bmp[];
extern char s_special_bmp[];
extern char s_spec_bar_bmp[];
extern char s_paused_bmp[];
extern char s_SkMedals[];

typedef struct {
	char *a;
	char *b;
	char pad[11];
	unsigned char c13;
	signed char c14;
	char pad2[319];
} E;

extern E Stats[];

typedef struct {
	int p0, p1, f8, fc;
	char pad[104];
} T;

extern T Levels[];

typedef struct {
	char pad[11];
	char f;
	char pad2[52];
} B;

extern B PSXRegion[];
extern int SelectedSkater;
extern char s_bits[];
extern char s_items[];
extern int GGame;
extern int SelectedSkater2;
extern int Cheat_BigHead;
extern int GNumberOfPlayers;
extern int pScoreSprite;
extern int pSpecialSprite;
extern int pBarSprite;
extern int DAT_800d1858;
extern char CurrentItemsFileRegion;
extern char CurrentMedalsFileRegion;
extern int BackOut;
extern int GLevel;
extern int GHeadPart;
extern int GHeadUp;

void Bit_Init(void);
void FUN_80025a88(void);
void Trig_ExecuteRestart(void);
void Trig_InitialParseTRGFile(char *);
void FUN_8004b09c(char *);
void FUN_8004b0e4(int);
void FUN_8004b180(void);
void FUN_8004bb5c(void);
void FUN_8004c740(int);
void FUN_8005dc8c(void);
int __6SpritePciii(int, char *, int, int, int);
void FUN_8006c4c4(void);
int Spool_PSX(char *, int, int);
void Font_Init(void);
int Front_FindLevel(char *);
void Front_SaveGameState(void);
void Redbook_InitialRand(int);
void SFX_SetReverbType(int);
void Spool_ClearAllPSXs(void);
void Spool_Sync(void);
void Trig_SetRestart(char *);
int __nw__6CClassUi(int);

void FUN_8001ccf8(char *p) {
	int a, b, s2, t1, t2, u1, u2, i1;
	char *bt, *bt2;
	E *q;
	char *bp, *bp2;
	B *tb, *tb2;

	TrigFileName = p + 4;
	Redbook_InitialRand(0x12a6cc58);
	FUN_8004c740(2000);
	FUN_8004b09c(s_skate2_vab);
	FUN_8004b0e4(Levels[GLevel].f8);
	SFX_SetReverbType(0);

	a = __nw__6CClassUi(24);

	if (a)
		a = __6SpritePciii(a, s_scorep_bmp, 1, 256, 256);

	pScoreSprite = a;
	a = __nw__6CClassUi(24);

	if (a)
		a = __6SpritePciii(a, s_special_bmp, 1, 256, 256);

	pSpecialSprite = a;
	a = __nw__6CClassUi(24);

	if (a)
		a = __6SpritePciii(a, s_spec_bar_bmp, 1, 256, 256);

	pBarSprite = a;
	b = __nw__6CClassUi(24);

	if (b)
		b = __6SpritePciii(b, s_paused_bmp, 1, 64, 256);

	DAT_800d1858 = b;
	i1 = Spool_PSX(s_bits, 0, 0);
	bt = (char *)PSXRegion;
	tb = (B *)(bt + i1 * 64);
	tb->f = 1;
	s2 = Spool_PSX(s_items, 0, 0);
	tb2 = (B *)(bt + s2 * 64);
	tb2->f = 1;
	CurrentItemsFileRegion = s2;
	Spool_Sync();
	FUN_8004c740(4);
	FUN_8006c4c4();
	Font_Init();
	Bit_Init();
	Spool_ClearAllPSXs();

	if (Levels[GLevel].fc && GGame == 1)
		s2 = Spool_PSX(s_SkMedals, 0, 0);

	CurrentMedalsFileRegion = s2;

	if (Cheat_BigHead) {
		bp = (char *)Stats;
		q = (E *)(bp + SelectedSkater * 340);
		t1 = q->c13;
		t2 = q->c14;
		GHeadPart = t1;
		GHeadUp = t2;
	}

	if (GNumberOfPlayers == 2 && GGame != 7)
		Spool_PSX(Stats[SelectedSkater].b, 0, 0);
	else
		Spool_PSX(Stats[SelectedSkater].a, 0, 0);

	Spool_Sync();

	GHeadPart = -1;

	if (GNumberOfPlayers == 2) {
		if (Cheat_BigHead) {
			bp2 = (char *)Stats;
			q = (E *)(bp2 + SelectedSkater2 * 340);
			u1 = q->c14;
			u2 = q->c13;
			GHeadUp = u1;
			GHeadPart = u2;
		}

		if (GGame != 7)
			Spool_PSX(Stats[SelectedSkater2].b, 0, 0);
		else
			Spool_PSX(Stats[SelectedSkater2].a, 0, 0);

		Spool_Sync();

		GHeadPart = -1;
	}

	if (Front_FindLevel(TrigFileName)) {
		FUN_8004bb5c();
		FUN_8004b180();
	}

	FUN_8004c740(2000);
	Trig_InitialParseTRGFile(TrigFileName);
	FUN_8005dc8c();
	FUN_8004c740(4);

	if (BackOut == 0) {
		FUN_80025a88();

		if (p[13])
			Trig_SetRestart(p + 13);

		Trig_ExecuteRestart();
		Front_SaveGameState();
	}
}
