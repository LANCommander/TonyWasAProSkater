/* Trig_SetRestart__FPc (0x80044ae4) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

int RestartNode, NumNodes;
short **OffsetList;

char *Trig_GetPosition(V *v, int n);
int Utils_CompareStrings__FPCcT0Ui(const char *a, const char *b, unsigned n);

void Trig_SetRestart(char *name) {
	int i;

	RestartNode = 0xffff;

	for (i = 0; i < NumNodes; i++) {
		if (*OffsetList[i] == 8) {
			V v;

			if (Utils_CompareStrings__FPCcT0Ui(Trig_GetPosition(&v, i) + 6, name, -1)) {
				RestartNode = i;
				return;
			}
		}
	}
}
