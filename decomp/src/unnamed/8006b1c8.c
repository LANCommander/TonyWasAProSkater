/* HiRes_SetupCamera__Fv (0x8006b1c8) */
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

void M3dMaths_SetIdentityRotation__FP6MATRIX(MATRIX *);
MATRIX *TransMatrix(MATRIX *, VECTOR *);
void M3dInit_SetFoggingParams(int, int, unsigned int);

SViewport *HiRes_SetupCamera(void) {
	DAT_800bf614.m = 3413;
	M3dMaths_SetIdentityRotation__FP6MATRIX(&MikeCamera.Transform);

	MikeCamera.Position.vx = 0;
	MikeCamera.Position.vy = 0;
	MikeCamera.Position.vz = -200;
	TransMatrix(&MikeCamera.Transform, &MikeCamera.Position);
	M3dInit_SetFoggingParams(10, 6000, 2048);
	return &DAT_800bf614;
}
