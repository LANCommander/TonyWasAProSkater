/* CalcPolyBufferEnd__Fv (0x80024258) */
/* CalcPolyBufferEnd: the demo's MAIN.CPP twin used a fixed POLYBUFFERSIZE; THPS1 sizes the
   primitive buffer per level and per player count. */
#include "game/m3d.h"
#include "game/career.h"

extern SDoubleBuffer *pDoubleBuffer; /* pDoubleBuffer */
extern int GLevel;                   /* GLevel */

int GNumberOfPlayers; /* number of players */

extern SLevel Levels[]; /* Levels */

unsigned long PolyBufferEnd; /* PolyBufferEnd */

#define POLYBUFFERSIZE (Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00)

void CalcPolyBufferEnd(void) {
	unsigned long Prims = (unsigned long)pDoubleBuffer->Polys;
	int Size = POLYBUFFERSIZE;

#ifdef PC_PORT
	/* the PS1 drew through KUSEG (bit 31 clear); on PC main RAM is only at 0x80000000 */
	PolyBufferEnd = (Prims + Size - 256) | 0x80000000;
#else
	PolyBufferEnd = (Prims + Size - 256) & 0x7fffffff;
#endif
}
