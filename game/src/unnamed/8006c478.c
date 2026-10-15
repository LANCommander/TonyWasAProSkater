/* Panel_addPrim__FPv (0x8006c478) */
#include "names.h"
#include "game/dbuffer.h"

extern int prim_added;

/* Adds a panel primitive to the 2D entry of the buffer being built. */
void Panel_addPrim(void *p) {
	addPrim(&pDoubleBuffer->OT2D, p);
	prim_added = 1;
}
