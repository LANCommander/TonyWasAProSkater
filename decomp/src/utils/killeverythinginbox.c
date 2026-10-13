/* Utils_KillEverythingInBox__FRC7CVectorT0 (0x800483e8) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

extern void *BaddyList, *EnvironmentalObjectList, *PowerUpList, *SuspendedList;

int Utils_KillObjectsInBox(V *a, V *b, void *list, int f);

int Utils_KillEverythingInBox(V *a, V *b) {
	int n = 0;

	n += Utils_KillObjectsInBox(a, b, BaddyList, 1);
	n += Utils_KillObjectsInBox(a, b, EnvironmentalObjectList, 1);
	n += Utils_KillObjectsInBox(a, b, PowerUpList, 1);
	n += Utils_KillObjectsInBox(a, b, SuspendedList, 0);
	return n;
}
