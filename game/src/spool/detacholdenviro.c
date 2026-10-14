/* DetachOldEnviro__FUc (0x8006fbe4) */
#include "names.h"

typedef struct {
	char name[10];
	unsigned char f0a, f0b;
	unsigned *models;
	unsigned *f10;
	char *f14;
	char pad[0x28];
} SRegion;

extern SRegion PSXRegion[];

typedef struct SEnviro {
	char pad[0x1c];
	struct SEnviro *next;
} SEnviro;

SEnviro *EnviroList;
int EnvRegions[2];
int pTexWibPacket[2];

void DetachOldEnviro(unsigned char idx) {
	SEnviro *other = 0;
	SEnviro *cur;
	int o;

	pTexWibPacket[idx] = 0;
	cur = (SEnviro *)(PSXRegion[EnvRegions[idx]].f14 + 12);
	o = EnvRegions[idx ^ 1];

	if (o != -1)
		other = (SEnviro *)(PSXRegion[o].f14 + 12);

	if (!other) {
		EnviroList = 0;
		return;
	}

	if (cur == EnviroList) {
		EnviroList = other;
		return;
	}

	{
		SEnviro *p = other;

		while (p->next != cur)
			p = p->next;

		p->next = 0;
	}
}
