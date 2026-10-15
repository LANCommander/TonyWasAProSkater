/* Display_Viewport__FP9SViewporti (0x80024ce4) */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/dbuffer.h"
#include "game/m3d.h"
#include "game/camera.h"
#include "game/bruce.h"

/* MAIN.CPP (demo twin). Renders one viewport: points the OT at its layer block, sets up the
   camera, then renders the item lists, skaters (with their shadow tint), bits and background. */
extern SCamera MikeCamera[];

extern int Mick_MapMode, Mick_MapDoit;

struct SEnviro;

extern CBody *PowerUpList, *BackgroundList, *BaddyList, *EnvironmentalObjectList;
extern SEnviro *EnviroList;
extern CBruce *MechList;
extern int Camera_CurrentMikeCamera;

extern int ViewportMode;
extern CBruce *DAT_800d17cc;

extern short M3d_OTPushback, M3d_OTPushback2;

extern CBruce *GSkater;

void Bit_Display();
void M3d_RenderBackground(void *p);
extern "C" void FUN_8002efa4(CBruce *);

void Display_Viewport(SViewport *a, int b) {
	short s1, s0;

	Camera_CurrentMikeCamera = b;
	M3d_RenderSetup(&MikeCamera[b], a, pDoubleBuffer->pOT = &pDoubleBuffer->OT3D[b * 4096]);

	Mick_MapDoit = Mick_MapMode;
	M3d_Render(EnviroList);

	Mick_MapDoit = 0;
	M3d_Render(EnvironmentalObjectList);
	FUN_8002efa4(GSkater);

	if (DAT_800d17cc)
		FUN_8002efa4(DAT_800d17cc);

	s1 = M3d_OTPushback;
	s0 = M3d_OTPushback2;

	if (GSkater->mPhysicsState && GSkater->mInAir == 0) {
		M3d_OTPushback = 128;
		M3d_OTPushback2 = 128;
	} else {
		M3d_OTPushback = 0;
		M3d_OTPushback2 = 0;
	}

	M3d_Render(MechList);
	GSkater->TempRestore();

	if (DAT_800d17cc)
		DAT_800d17cc->TempRestore();

	M3d_OTPushback = s1;
	M3d_OTPushback2 = s0;
	Bit_Display();
	M3d_Render(BaddyList);
	M3d_Render(PowerUpList);

	if (ViewportMode == 0)
		M3d_RenderBackground(BackgroundList);

	M3d_RenderCleanup();
}
