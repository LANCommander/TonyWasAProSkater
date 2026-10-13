/* Utils_LineOfSight__FRC7CVectorT0 (0x80048170) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

typedef struct {
	V start, end;
	char pad[0x68 - 24];
	int hit;
	char pad2[0xa8 - 0x6c];
} SLineInfo;

extern int LineOfSightCheck;

void M3dColij_InitLineInfo(SLineInfo *li);
void M3dZone_LineToItemNotSCached(SLineInfo *li, int n);

int Utils_LineOfSight(V *a, V *b) {
	SLineInfo li;

	li.start.x = a->x;
	li.start.y = a->y;
	li.start.z = a->z;
	li.end.x = b->x;
	li.end.y = b->y;
	li.end.z = b->z;
	LineOfSightCheck = 1;
	M3dColij_InitLineInfo(&li);
	M3dZone_LineToItemNotSCached(&li, 1);

	LineOfSightCheck = 0;
	return li.hit == 0;
}
