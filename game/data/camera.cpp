/* Decompiled data: the retail executable's initialised globals that src/camera/ uses, each at
 * its retail address (psxdata.h). Text is string literals where it's used and the tables have
 * the game's types; platform/pc/tools/data_image.py checks it all against the retail bytes. */
#include "psxdata.h"
#include "game/camera.h"

extern char dtor_t9AnimParam1Z5CQuat[] PSX_NAME("_._t9AnimParam1Z5CQuat");

PSX_DATA(0x800a3594) PsxVtEntry DAT_800a3594[2] = {
	{0, 0, 0},
	{0, 0, dtor_t9AnimParam1Z5CQuat},
};

/* SPresetCamera[5] (THPS2 demo: SPresetCamera[5]) */
PSX_DATA(0x800c1118) SPresetCamera Presets[5] = {
	{-2000, 654, 6786, {}, "Very Far"},
	{-268, 0, 946, {}, "Far"},
	{-140, 118, 386, {}, "Default"},
	{-60, 80, 202, {}, "Close"},
	{-124, 0, 0, {}, "Headshot"},
};

/* CVector (THPS2 demo: CVector) */
PSX_DATA(0x800c1154) int Camera_CamOffset[3];

/* MATRIX (THPS2 demo: MATRIX) */
PSX_DATA(0x800c12e4) int tm[8];

/* THPS2 demo: int */
PSX_DATA(0x800d1728) int Cheat_ScreenShots[1];

/* THPS2 demo: int */
PSX_DATA(0x800d1ff4) int camDist[1] = {
	658,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1ff8) int camPitch[1] = {
	54,
};

/* THPS2 demo: int */
PSX_DATA(0x800d1ffc) int camYOffset[9] = {
	-220, 0, -212,
};

/* THPS2 demo: int */
PSX_DATA(0x800d2020) int Camera_Preset[1] = {
	2,
};

/* THPS2 demo: int */
PSX_DATA(0x800d2038) int Camera_CamPitch[1] = {
	54,
};

/* THPS2 demo: int */
PSX_DATA(0x800d203c) int Camera_CamDist[1] = {
	850,
};

/* THPS2 demo: int */
PSX_DATA(0x800d2040) int NumCameras[1];

/* THPS2 demo: uint */
PSX_DATA(0x800d2048) unsigned int Camera_CurrentMikeCamera[8] = {
	0, 0x20676962, 0x726961, 0x706d6172, 0x676e69, 0x6d726f6e, 0x6c61,
};

/* THPS2 demo: int */
PSX_DATA(0x800d2080) int Mick_CameraTweak[1];

PSX_DATA(0x800d2084) int DAT_800d2084[1] = {
	1,
};

PSX_DATA(0x800d2088) int DAT_800d2088[1] = {
	256,
};

PSX_DATA(0x800d208c) int DAT_800d208c[1];
PSX_DATA(0x800d2090) int DAT_800d2090[2];

/* THPS2 demo: int */
PSX_DATA(0x800d20cc) int CameraCollisionCheck[1];
