/* CalcPolyBufferEnd__Fv (0x80024258) */
/* CalcPolyBufferEnd: the demo's MAIN.CPP twin used a fixed POLYBUFFERSIZE; THPS1 sizes the
   primitive buffer per level and per player count. */
#include "game/m3d.h"
#include "game/career.h"

extern SDoubleBuffer *pDoubleBuffer; /* pDoubleBuffer */
extern int GLevel;                   /* GLevel */

int GNumberOfPlayers; /* number of players */

extern SLevel Levels[]; /* Levels */

unsigned char *PolyBufferEnd; /* PolyBufferEnd */

#define POLYBUFFERSIZE (Levels[GLevel].ExtraPolyBuffer[GNumberOfPlayers - 1] + 0x17c00)

void CalcPolyBufferEnd(void) {
	unsigned char *Prims = pDoubleBuffer->Polys;
	int Size = DB_PACKET_MEMORY(POLYBUFFERSIZE);

	PolyBufferEnd = Prims + Size - 256;
}
