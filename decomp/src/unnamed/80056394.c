/* Front_ResetDrawArea2P__Fv (0x80056394) */
#include "names.h"
/* Full-screen DR_AREA added at OT slot 39; Front_ResetDrawArea1P is a byte-identical copy. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

extern u_long *pDoubleBuffer;
extern u_char *pPoly;

void Front_ResetDrawArea2P(void) {
	DR_AREA *p = (DR_AREA *)pPoly;
	RECT r;

	setRECT(&r, 0, 0, 640, 480);
	SetDrawArea(p, &r);

	{
		u_long *ot = pDoubleBuffer + 39;

		addPrim(ot, p);
	}

	pPoly = (u_char *)(p + 1);
}
