/* Spool_FindEnviroItem__FUi (0x8006ee88) */
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
	char pad[0x16];
	unsigned short model;
	char pad2[3];
	unsigned char region;
	struct SEnviro *next;
} SEnviro;

SEnviro *EnviroList;

SEnviro *Spool_FindEnviroItem(unsigned id) {
	SEnviro *e = EnviroList;
	int cur = -1;
	unsigned *tbl;

	while (e) {
		if (e->region != cur) {
			cur = e->region;
			tbl = PSXRegion[cur].models;
		}

		if (tbl[e->model] == id)
			return e;

		e = e->next;
	}

	return 0;
}
