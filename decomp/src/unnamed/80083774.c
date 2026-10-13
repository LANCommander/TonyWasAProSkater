/* M3d_RenderCleanup__Fv (0x80083774) */
#include "names.h"
/* Clips drawing to the current zone's screen box with a DR_AREA at the last OT slot (4095). */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>

typedef struct {
	short x1, y1, x0, y0;
} SBox;

typedef struct {
	short pad0, y;
} SDispBuf;

typedef struct {
	u_long OT[4096];
} SOT;

SBox *pCurrentViewport;
SOT *CurrentOrderingTable;

extern SDispBuf *pDoubleBuffer;
extern u_char *pPoly;

void M3d_RenderCleanup(void) {
	RECT r;
	DR_AREA *p;

	r.x = pCurrentViewport->x0;
	r.y = pCurrentViewport->y0 + pDoubleBuffer->y;
	p = (DR_AREA *)pPoly;
	r.w = pCurrentViewport->x1 - pCurrentViewport->x0;
	r.h = pCurrentViewport->y1 - pCurrentViewport->y0;
	SetDrawArea(p, &r);

	{
		u_long *ot = &CurrentOrderingTable->OT[4095];

		addPrim(ot, p);
	}

	pPoly = (u_char *)(p + 1);
}
