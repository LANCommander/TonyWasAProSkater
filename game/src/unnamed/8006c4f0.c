/* Panel_DrawTexturedPoly__FP10SAnimFrame (0x8006c4f0) */
#include "names.h"
/* Allocates a grey-modulated POLY_FT4, copies its UVs/CLUT/TPAGE from the texture record and
   adds it at OT slot 33 (coordinates are left to the caller). Returns 0 when the primitive buffer
   is full. */
#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "game/dbuffer.h"

typedef struct {
	u_long uv0clut, uv1tpage;
	u_short uv2, uv3;
} STexUV;

typedef struct {
	int pad;
	STexUV *pUV;
} STexture;

extern u_char *pPoly;
extern u_char *PolyBufferEnd;

POLY_FT4 *Panel_DrawTexturedPoly(STexture *t) {
	POLY_FT4 *p = (POLY_FT4 *)pPoly;
	u_char *next = (u_char *)(p + 1);
	STexUV *uv;

	if (next > PolyBufferEnd)
		return 0;

	pPoly = next;
	setlen(p, 9);
	*(u_long *)&p->r0 = 0x2c808080;
	uv = t->pUV;

	{
		u_long a = uv->uv0clut;
		u_long b = uv->uv1tpage;
		u_long c = *(u_long *)&uv->uv2;
		u_short d = uv->uv3;

		*(u_long *)&p->u0 = a;
		*(u_long *)&p->u1 = b;
		*(u_long *)&p->u2 = c;
		*(u_short *)&p->u3 = d;
	}

	addPrim(&pDoubleBuffer->OT2D, p);

	return p;
}
