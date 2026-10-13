/* SendUnSuspend__Fi (0x80045e98) */
#include "names.h"

extern char *SuspendedList;

void CBody_UnSuspend(char *b);

void SendUnSuspend(int node) {
	char *b;

	for (b = SuspendedList; b; b = *(char **)(b + 0x1c))
		if (*(unsigned short *)(b + 0xe2) == node)
			CBody_UnSuspend(b);
}
