/* Rail_NextNode__FPUs (0x8004f578) */
#include "names.h"

extern short **OffsetList;

int Rail_NextNode(unsigned short *p) {
	unsigned a;

	if (p[-1] == 11) {
		a = p[1];

		if (*OffsetList[a] == 10)
			return a;

		return p[2];
	}

	return p[1];
}
