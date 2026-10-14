/* Db_Reset__Fv (0x800491c4) */
#include "names.h"
#include "game/dbuffer.h"

/* Forgets both buffers' packet memory. */
void Db_Reset(void) {
	DoubleBuffer[0].Polys = 0;
	DoubleBuffer[1].Polys = 0;
}
