/* Display_Viewport__FP9SViewporti (0x80024ce4) */
#include "names.h"

/* MAIN.CPP, as C (demo twin). Renders one viewport: points the OT at its layer block, sets up the
   camera, then renders the item lists, skaters (with their shadow tint), bits and background. */
typedef struct {
	char p[0x358];
	int f358;
	char q[0x734 - 0x35c];
	int f734;
} B;

extern char MikeCamera[];

int Mick_MapMode, Mick_MapDoit;

extern int PowerUpList, EnviroList, BackgroundList, Camera_CurrentMikeCamera, BaddyList, MechList,
    EnvironmentalObjectList;

int ViewportMode;
B *DAT_800d17cc;

extern short M3d_OTPushback, M3d_OTPushback2;

typedef struct {
	char pad[140];
	unsigned long Layer[8192];
	unsigned long *pCurrent;
} OTB;

extern OTB *pDoubleBuffer;

B *GSkater;

void FUN_8002ab20(void);
void FUN_8002efa4(B *);
void M3d_RenderSetup(char *, int, char *);
void FUN_80083774(void);
void M3d_Render(int);
void M3d_RenderBackground(int);
void CBruce_TempRestore(B *);

void Display_Viewport(int a, int b) {
	short s1, s0;

	Camera_CurrentMikeCamera = b;
	M3d_RenderSetup(MikeCamera + b * 164, a,
	                (char *)(pDoubleBuffer->pCurrent = &pDoubleBuffer->Layer[b * 4096]));

	Mick_MapDoit = Mick_MapMode;
	M3d_Render(EnviroList);

	Mick_MapDoit = 0;
	M3d_Render(EnvironmentalObjectList);
	FUN_8002efa4(GSkater);

	if (DAT_800d17cc)
		FUN_8002efa4(DAT_800d17cc);

	s1 = M3d_OTPushback;
	s0 = M3d_OTPushback2;

	if (GSkater->f734 && GSkater->f358 == 0) {
		M3d_OTPushback = 128;
		M3d_OTPushback2 = 128;
	} else {
		M3d_OTPushback = 0;
		M3d_OTPushback2 = 0;
	}

	M3d_Render(MechList);
	CBruce_TempRestore(GSkater);

	if (DAT_800d17cc)
		CBruce_TempRestore(DAT_800d17cc);

	M3d_OTPushback = s1;
	M3d_OTPushback2 = s0;
	FUN_8002ab20();
	M3d_Render(BaddyList);
	M3d_Render(PowerUpList);

	if (ViewportMode == 0)
		M3d_RenderBackground(BackgroundList);

	FUN_80083774();
}
