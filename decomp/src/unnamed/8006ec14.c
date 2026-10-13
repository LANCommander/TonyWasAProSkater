/* Spool_Init__Fv (0x8006ec14) */
#include "names.h"

typedef struct {
	char a0;
	char pad0[9];
	char a10, a11;
	char pad1[4];
	int f16, f20, f24, f28, f32, f36, f40, f44;
	char pad2[3];
	char a51, a52;
	char pad3[11];
} T;

extern T PSXRegion[];
extern char SpoolQ_d[][17];
extern int TextureChecksumHashTable[];

int SpoolPos;
int FreePos;
int AnimPackets;
int EnvRegions;
int EnvRegions_4;
int DAT_800d1ed8;
int DAT_800d1edc;
int EnviroList;
int OtherEnvIndex;

void Spool_Init(void) {
	int i;

	for (i = 0; i < 20; i++)
		SpoolQ_d[i][0] = 0;

	SpoolPos = 0;
	FreePos = 0;

	for (i = 0; i < 40; i++) {
		PSXRegion[i].a0 = 0;
		PSXRegion[i].a11 = 0;
		PSXRegion[i].a10 = 0;
		PSXRegion[i].f16 = 0;
		PSXRegion[i].f20 = 0;
		PSXRegion[i].f24 = 0;
		PSXRegion[i].f28 = 0;
		PSXRegion[i].f44 = 0;
		PSXRegion[i].f40 = 0;
		PSXRegion[i].f32 = 0;
		PSXRegion[i].a51 = 0;
		PSXRegion[i].a52 = 0;
	}

	for (i = 0; i < 512; i++)
		TextureChecksumHashTable[i] = 0;

	AnimPackets = 0;
	EnvRegions = -1;
	EnvRegions_4 = -1;
	DAT_800d1ed8 = 0;
	DAT_800d1edc = 0;
	EnviroList = 0;
	OtherEnvIndex = 0;
}
