/* FUN_8006b1c8 (0x8006b1c8) */
#include "names.h"

typedef struct {
	short m[3][3];
	int t[3];
} MATRIX;

typedef struct {
	int vx, vy, vz, pad;
} VECTOR;

typedef struct {
	unsigned int Style;
	VECTOR Position;
	VECTOR Focus;
	int Distance;
	int Height;
	short Angles[4];
	MATRIX Transform;
} SCamera;

typedef struct {
	char pad[12];
	short m;
} SViewport;

extern SCamera MikeCamera;
extern SViewport DAT_800bf614;

void FUN_8008b238(MATRIX *);
MATRIX *FUN_80093100(MATRIX *, VECTOR *);
void M3dInit_SetFoggingParams(int, int, unsigned int);

SViewport *FUN_8006b1c8(void) {
	DAT_800bf614.m = 3413;
	FUN_8008b238(&MikeCamera.Transform);

	MikeCamera.Position.vx = 0;
	MikeCamera.Position.vy = 0;
	MikeCamera.Position.vz = -200;
	FUN_80093100(&MikeCamera.Transform, &MikeCamera.Position);
	M3dInit_SetFoggingParams(10, 6000, 2048);
	return &DAT_800bf614;
}
