/* CamPt_New__Fi (0x8004e21c) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

extern short CamPtNode[];

extern int CamPtIndex;

void Trig_GetPosition(V *v, int n);

void CamPt_New(int n) {
	V v;

	Trig_GetPosition(&v, n);

	CamPtNode[CamPtIndex++] = n;
}
