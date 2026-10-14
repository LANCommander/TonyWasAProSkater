/* SendSuspend__FPP5CBodyi (0x80045e30) */
#include "names.h"

void CBody_Suspend(char *b, char **pp);

void SendSuspend(char **pp, int node) {
	char *b;

	for (b = *pp; b; b = *(char **)(b + 0x1c))
		if (*(unsigned short *)(b + 0xe2) == node)
			CBody_Suspend(b, pp);
}
