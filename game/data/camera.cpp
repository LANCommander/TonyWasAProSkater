/* Decompiled data: the retail executable's initialised globals that src/camera/ uses (psxdata.h).
 * Text is string literals where it's used and the tables have the game's types. */
#include "psxdata.h"
#include "game/camera.h"

extern char dtor_t9AnimParam1Z5CQuat[] PSX_NAME("_._t9AnimParam1Z5CQuat");

PsxVtEntry DAT_800a3594[2] = {
	{0, 0, 0},
	{0, 0, dtor_t9AnimParam1Z5CQuat},
};

/* SPresetCamera[5] (THPS2 demo: SPresetCamera[5]) */
SPresetCamera Presets[5] = {
	{-2000, 654, 6786, {}, "Very Far"},
	{-268, 0, 946, {}, "Far"},
	{-140, 118, 386, {}, "Default"},
	{-60, 80, 202, {}, "Close"},
	{-124, 0, 0, {}, "Headshot"},
};

/* CVector (THPS2 demo: CVector) */
CVector Camera_CamOffset;

/* MATRIX (THPS2 demo: MATRIX) */
MATRIX tm;

/* THPS2 demo: int */
int Cheat_ScreenShots;

/* THPS2 demo: int */
int camDist = 658;

/* THPS2 demo: int */
int camPitch = 54;

/* THPS2 demo: int */
int camYOffset = -220;

/* THPS2 demo: int */
int Camera_Preset = 2;

/* THPS2 demo: int */
int Camera_CamPitch = 54;

/* THPS2 demo: int */
int Camera_CamDist = 850;

/* THPS2 demo: int */
int NumCameras;

/* THPS2 demo: uint */
uint Camera_CurrentMikeCamera = 0;

/* THPS2 demo: int */
int Mick_CameraTweak;

int DAT_800d2084 = 1;

int DAT_800d2088 = 256;

int DAT_800d208c;
int DAT_800d2090;

/* THPS2 demo: int */
int CameraCollisionCheck;
