/* AttachNewEnviro__Fi (0x8006f7a4) */
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
int pCurrentTexWibPacket;
int pTexWibPacket[2];

int Spool_GetEnvIndex(int);

void AttachNewEnviro(int region) {
	if (!EnviroList) {
		EnviroList = (SEnviro *)(PSXRegion[region].f14 + 12);
	} else {
		SEnviro *head = (SEnviro *)(PSXRegion[region].f14 + 12);
		SEnviro *p = head;

		while (p->next)
			p = p->next;

		p->next = EnviroList;
		EnviroList = head;
	}

	pTexWibPacket[Spool_GetEnvIndex(region)] = pCurrentTexWibPacket;
}
