/* CollideWithSkaters__FP4CCar (0x8006bc88) */
#include "names.h"
/* Builds the object's rotation matrix and runs CollideWithSkater against the skater and its partner
   (+0x1c), unless a replay/video mode 2 is active on either player. */
#include <sys/types.h>
#include <libgte.h>

typedef struct {
	int VideoMode;
	char pad[0xc0 - 4];
} SVideoInfo;

typedef struct {
	char pad[16];
	char *p10;
} SModelInfo;

typedef struct {
	char pad[16];
	SModelInfo *pInfo;
	char q[64 - 20];
} SType;

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern SType PSXRegion[];
extern char *MechList;

void CollideWithSkater(char *, char *, MATRIX *, char *);

void CollideWithSkaters(char *p) {
	MATRIX m;
	SModelInfo *info;
	char *s1, *s2, *x;

	if (GVInfo1.VideoMode == 2 || GVInfo2.VideoMode == 2)
		return;

	info = PSXRegion[(unsigned char)p[27]].pInfo;
	s1 = MechList;
	s2 = *(char **)(s1 + 28);
	x = info->p10 + 12;
	RotMatrix((SVECTOR *)(p + 356), &m);
	CollideWithSkater(p, s1, &m, x);

	if (s2)
		CollideWithSkater(p, s2, &m, x);
}
